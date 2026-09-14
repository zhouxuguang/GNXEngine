//
//  DX12Buffer.h
//  rendercore
//
//  D3D12 缓冲区实现（RCBuffer / VertexBuffer / IndexBuffer / UniformBuffer）。
//
//  ── StorageModeShared 的语义映射（D3D12 与 Vulkan/Metal 差异最大的地方）──
//  引擎用 StorageModeShared 同时表达两类需求：
//    (a) CPU 需要 Map() 写入/读取（ImGui 顶点缓冲、地形 patch 数据、VT 回读暂存）
//    (b) GPU 需要以 UAV 写入（compute 剔除结果、间接参数）
//  而 D3D12 的 UPLOAD 堆不允许创建 UAV，READBACK 堆不能被着色器访问。
//  因此按用途细分：
//    - 含 StorageBuffer 用途        → DEFAULT 堆（可 UAV/SRV），CPU 访问走暂存缓冲
//    - 仅 TransferDst（回读用）     → READBACK 堆（GPU 写入、CPU 读取）
//    - 其余 + StorageModeShared     → UPLOAD 堆（CPU 持续映射、GPU 只读）
//    - StorageModePrivate           → DEFAULT 堆
//

#ifndef GNX_ENGINE_DX12_BUFFER_INCLUDE_SDFHG
#define GNX_ENGINE_DX12_BUFFER_INCLUDE_SDFHG

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "RCBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// 缓冲区公共基础设施
// ============================================================================
class DX12BufferBase
{
public:
    DX12BufferBase() = default;
    virtual ~DX12BufferBase();

    DX12BufferBase(const DX12BufferBase&) = delete;
    DX12BufferBase& operator=(const DX12BufferBase&) = delete;

    bool IsResourceValid() const { return mResource != nullptr; }

    ID3D12Resource* GetResource() const { return mResource.Get(); }
    D3D12MA::Allocation* GetAllocation() const { return mAllocation; }
    uint32_t GetSizeInBytes() const { return mSize; }
    RCBufferUsage GetUsageFlags() const { return mUsage; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return mGPUAddress; }
    D3D12_RESOURCE_STATES GetCurrentState() const { return mCurrentState; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { mCurrentState = state; }

    /// CPU 可访问的指针（UPLOAD/READBACK 为持久映射；DEFAULT 为暂存缓冲的映射）
    void* GetCpuPointer() const;

    /// 是否需要把 CPU 侧的改动同步回 GPU 资源（仅 DEFAULT 堆的 StorageBuffer）
    bool NeedsUpload() const { return mNeedsUpload && mStagingResource != nullptr; }
    void ClearUploadFlag() { mNeedsUpload = false; }

    void SetDebugName(const char* name);

protected:
    bool CreateBufferInternal(const DX12ContextPtr& context, const RCBufferDesc& desc,
                              const void* initialData);

    D3D12_HEAP_TYPE ChooseHeapType(const RCBufferDesc& desc) const;

    /// 暂存缓冲的方向：写回 GPU（CPU→GPU）或回读（GPU→CPU）
    enum class StagingDirection
    {
        WriteThrough,   // CPU 写入 → 同步到 GPU 资源
        Readback,       // GPU 资源 → 同步到暂存，供 CPU 读取
    };

    /// 把暂存内容同步到 GPU 资源（DEFAULT 堆 + CPU 写入场景）
    void SyncStagingToResource();

    /// 把 GPU 资源内容同步到暂存（DEFAULT 堆 + CPU 回读场景）
    void SyncResourceToStaging();

    DX12ContextPtr mContext;
    ComPtr<ID3D12Resource> mResource;
    D3D12MA::Allocation* mAllocation = nullptr;

    // CPU 暂存（仅 DEFAULT 堆且需要 CPU 访问时创建）
    ComPtr<ID3D12Resource> mStagingResource;
    D3D12MA::Allocation* mStagingAllocation = nullptr;
    D3D12_HEAP_TYPE mStagingHeapType = D3D12_HEAP_TYPE_UPLOAD;
    void* mStagingMapped = nullptr;

    void* mPersistentMapped = nullptr;
    bool mNeedsUpload = false;
    StagingDirection mStagingDirection = StagingDirection::WriteThrough;

    uint32_t mSize = 0;
    RCBufferUsage mUsage = RCBufferUsage::Unknown;
    StorageMode mStorageMode = StorageModePrivate;
    D3D12_RESOURCE_STATES mCurrentState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress = 0;
    std::string mDebugName;

    friend class DX12CommandBuffer;
};

// ============================================================================
// RCBuffer
// ============================================================================
class DX12RCBuffer : public RCBuffer, public DX12BufferBase
{
public:
    DX12RCBuffer(const DX12ContextPtr& context, const RCBufferDesc& desc);
    DX12RCBuffer(const DX12ContextPtr& context, const RCBufferDesc& desc, const void* data);
    ~DX12RCBuffer() override = default;

    uint32_t GetSize() const override { return mSize; }
    RCBufferUsage GetUsage() const override { return mUsage; }

    void* Map() const override;
    void Unmap() const override;

    bool IsValid() const override { return IsResourceValid(); }
    void SetName(const char* name) override { SetDebugName(name); }

    /// 该缓冲区是否需要以 SRV 形式绑定（结构化缓冲 / 原始缓冲）
    bool SupportsSRV() const { return IsResourceValid(); }

    /// 该缓冲区是否需要以 UAV 形式绑定
    bool SupportsUAV() const;
};

using DX12RCBufferPtr = std::shared_ptr<DX12RCBuffer>;

// ============================================================================
// VertexBuffer（旧接口，内部持有 RCBuffer）
// ============================================================================
class DX12VertexBuffer : public VertexBuffer, public DX12BufferBase
{
public:
    explicit DX12VertexBuffer(const DX12ContextPtr& context, uint32_t size, StorageMode mode);
    DX12VertexBuffer(const DX12ContextPtr& context, const void* buffer, uint32_t size, StorageMode mode);
    ~DX12VertexBuffer() override = default;

    uint32_t GetBufferLength() const override { return mSize; }
    void* MapBufferData() const override { return GetCpuPointer(); }
    void UnmapBufferData(void* bufferData) const override { (void)bufferData; }
    bool IsValid() const override { return IsResourceValid(); }
    void SetName(const char* name) override { SetDebugName(name); }
};

// ============================================================================
// IndexBuffer
// ============================================================================
class DX12IndexBuffer : public IndexBuffer, public DX12BufferBase
{
public:
    DX12IndexBuffer(const DX12ContextPtr& context, IndexType indexType,
                    const void* data, uint32_t dataLen);
    ~DX12IndexBuffer() override = default;

    IndexType GetIndexType() const { return mIndexType; }
    DXGI_FORMAT GetDXGIIndexFormat() const;

private:
    IndexType mIndexType = IndexType_UShort;
};

using DX12IndexBufferPtr = std::shared_ptr<DX12IndexBuffer>;

// ============================================================================
// UniformBuffer
//
// 引擎把 UBO 当作“一次性写入的常量块”，DX12 下直接映射到 UPLOAD 堆的
// 常量缓冲区，绑定时创建 CBV 指向它的 GPU 地址。
// ============================================================================
class DX12UniformBuffer : public UniformBuffer, public DX12BufferBase
{
public:
    DX12UniformBuffer(const DX12ContextPtr& context, uint32_t size);
    ~DX12UniformBuffer() override = default;

    void SetData(const void* data, uint32_t offset, uint32_t dataSize) override;

    /// 保持与 Vulkan 后端一致的“影子数据”语义（部分路径会读取它）
    const void* GetShadowData() const { return mShadowData.data(); }
    uint32_t GetAlignedSize() const { return mAlignedSize; }

private:
    std::vector<uint8_t> mShadowData;
    uint32_t mAlignedSize = 0;
};

using DX12UniformBufferPtr = std::shared_ptr<DX12UniformBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_BUFFER_INCLUDE_SDFHG */
