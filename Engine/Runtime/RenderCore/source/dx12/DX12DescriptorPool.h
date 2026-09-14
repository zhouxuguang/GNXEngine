//
//  DX12DescriptorPool.h
//  rendercore
//
//  描述符堆 + 单帧线性分配器。
//
//  D3D12 与 Vulkan 的描述符模型差异很大：Vulkan 后端使用
//  vkCmdPushDescriptorSetKHR，每次绑定直接写进命令流；D3D12 则必须把描述符
//  写进 shader-visible 堆，再把表基址绑到根签名上。
//
//  这里采用“单帧线性分配”策略：
//    - 每帧从堆里 bump 分配连续的描述符块（块内下标即 HLSL 寄存器号）；
//    - 块在 GPU 完成该帧后随 Reset() 回收；
//    - 引擎的绑定顺序是 [SetPipeline][Set*...][Draw]，因此在每次 Draw 之后
//      把当前块标记为已消费，下一次 Set* 会重新分配新块。
//      这样“连续多次 Draw 复用同一组绑定”和“每次 Draw 重新绑定”两种模式都正确。
//

#ifndef GNX_ENGINE_DX12_DESCRIPTOR_POOL_INCLUDE_JHGFD
#define GNX_ENGINE_DX12_DESCRIPTOR_POOL_INCLUDE_JHGFD

#include "DX12RenderDefine.h"
#include "DX12Helpers.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12DescriptorPool
{
public:
    DX12DescriptorPool() = default;
    ~DX12DescriptorPool();

    DX12DescriptorPool(const DX12DescriptorPool&) = delete;
    DX12DescriptorPool& operator=(const DX12DescriptorPool&) = delete;

    /**
     * @brief 创建描述符堆
     * @param device D3D12 设备
     * @param type 堆类型（CBV_SRV_UAV / SAMPLER / RTV / DSV）
     * @param capacity 描述符数量
     * @param shaderVisible 是否允许着色器可见（RTV/DSV 必须为 false）
     * @param debugName 调试名
     */
    bool Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity,
              bool shaderVisible, const char* debugName);

    void Destroy();

    /**
     * @brief 分配一段连续描述符
     * @param count 需要的描述符数量
     * @param outStartIndex 输出起始下标
     * @return 成功返回 true；堆耗尽时返回 false（内部会计数并在首次溢出时打印错误）
     */
    bool Allocate(uint32_t count, uint32_t& outStartIndex);

    /// 回收整块（在 GPU 完成对应帧之后调用）
    void Reset();

    ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }
    bool IsValid() const { return mHeap != nullptr; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;

    uint32_t GetCapacity() const { return mCapacity; }
    uint32_t GetIncrementSize() const { return mIncrementSize; }
    uint32_t GetUsedCount() const { return mUsedCount; }
    uint32_t GetPeakUsedCount() const { return mPeakUsedCount; }
    uint64_t GetOverflowCount() const { return mOverflowCount; }

private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mCpuStart = {};
    D3D12_GPU_DESCRIPTOR_HANDLE mGpuStart = {};
    D3D12_DESCRIPTOR_HEAP_TYPE  mType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uint32_t mCapacity = 0;
    uint32_t mIncrementSize = 0;
    uint32_t mUsedCount = 0;
    uint32_t mPeakUsedCount = 0;
    uint64_t mOverflowCount = 0;
    bool mShaderVisible = false;
    bool mOverflowLogged = false;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_DESCRIPTOR_POOL_INCLUDE_JHGFD */
