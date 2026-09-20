//
//  DX12Buffer.cpp
//  rendercore
//

#include "DX12Buffer.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// DX12BufferBase
// ============================================================================

DX12BufferBase::~DX12BufferBase()
{
    if (mResource != nullptr)
    {
        // DEFAULT 堆资源在 Unmap 之前可能仍有未同步的 CPU 改动；
        // 这里不做隐式同步（对象销毁时已经没有可用的命令列表上下文）。
        mResource.Reset();
    }

    if (mStagingResource != nullptr)
    {
        mStagingResource->Unmap(0, nullptr);
        mStagingMapped = nullptr;
        mStagingResource.Reset();
    }

    if (mContext != nullptr && mContext->allocator != nullptr)
    {
        if (mAllocation != nullptr)
        {
            mContext->ReleaseAllocation(mAllocation);
            mAllocation = nullptr;
        }
        if (mStagingAllocation != nullptr)
        {
            mContext->ReleaseAllocation(mStagingAllocation);
            mStagingAllocation = nullptr;
        }
    }
}

D3D12_HEAP_TYPE DX12BufferBase::ChooseHeapType(const RCBufferDesc& desc) const
{
    if (desc.storageMode == StorageModePrivate)
    {
        return D3D12_HEAP_TYPE_DEFAULT;
    }

    // 含 StorageBuffer：必须放在 DEFAULT 堆，因为可能被创建为 UAV
    // （D3D12 不允许在 UPLOAD 堆上创建 UAV）。
    if (HasUsage(desc.usage, RCBufferUsage::StorageBuffer))
    {
        return D3D12_HEAP_TYPE_DEFAULT;
    }

    // 纯回读目标（VT feedback 的暂存缓冲）：GPU 写入、CPU 读取
    if (HasUsage(desc.usage, RCBufferUsage::TransferDst))
    {
        return D3D12_HEAP_TYPE_READBACK;
    }

    // 其余共享缓冲：CPU 持续写入、GPU 只读（ImGui 动态顶点缓冲、普通 UBO 等）
    return D3D12_HEAP_TYPE_UPLOAD;
}

bool DX12BufferBase::CreateBufferInternal(const DX12ContextPtr& context, const RCBufferDesc& desc,
                                          const void* initialData)
{
    mContext = context;
    mUsage = desc.usage;
    mStorageMode = desc.storageMode;

    if (!mContext || !mContext->IsValid())
    {
        LOG_ERROR("[DX12] CreateBufferInternal: invalid context");
        return false;
    }

    const D3D12_HEAP_TYPE heapType = ChooseHeapType(desc);

    // ---- 资源标志：StorageBuffer 需要允许 UAV ----
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if (HasUsage(desc.usage, RCBufferUsage::StorageBuffer))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if (HasUsage(desc.usage, RCBufferUsage::VertexBuffer) ||
        HasUsage(desc.usage, RCBufferUsage::IndexBuffer))
    {
        // 顶点/索引缓冲不会被同时当作 RT/DS，无需额外标志
    }

    // ---- 初始状态（保证首次 barrier 的 StateBefore 合法）----
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    switch (heapType)
    {
        case D3D12_HEAP_TYPE_UPLOAD:   initialState = D3D12_RESOURCE_STATE_GENERIC_READ; break;
        case D3D12_HEAP_TYPE_READBACK: initialState = D3D12_RESOURCE_STATE_COPY_DEST;    break;
        default:                       initialState = D3D12_RESOURCE_STATE_COMMON;       break;
    }

    // ---- 创建主资源（16 字节对齐，满足 constant buffer / CBV 要求）----
    mSize = desc.size;
    const uint64_t allocSize = (DX12Util::AlignUp(desc.size, 16));

    const D3D12_RESOURCE_DESC resourceDesc = DX12BufferResourceDesc(allocSize, flags);
    HRESULT hr = mContext->CreateResource(resourceDesc, heapType, initialState, &mAllocation,
                                          IID_PPV_ARGS(&mResource), nullptr, mDebugName.c_str());
    if (FAILED(hr) || mResource == nullptr)
    {
        LOG_ERROR("[DX12] Failed to create buffer (%u bytes, heap=%d, usage=0x%X): %s",
                  desc.size, (int)heapType, (unsigned)desc.usage, DX12HResultToString(hr));
        return false;
    }

    mGPUAddress = mResource->GetGPUVirtualAddress();
    mCurrentState = initialState;

    // ---- CPU 可见的堆：持久映射 ----
    if (heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_READBACK)
    {
        const D3D12_RANGE readRange = DX12ReadRange(0, 0);
        void* mapped = nullptr;
        if (SUCCEEDED(mResource->Map(0, &readRange, &mapped)))
        {
            mPersistentMapped = mapped;
        }
        else
        {
            LOG_WARN("[DX12] Failed to persistently map buffer (heap=%d)", (int)heapType);
        }
    }
    // ---- DEFAULT 堆 + 需要 CPU 访问：额外创建一个暂存缓冲 ----
    else if (desc.storageMode == StorageModeShared)
    {
        // 带 Transfer 用途说明这是拷贝的源/目标，CPU 侧需要读到 GPU 的结果
        const bool needsReadback = HasUsage(desc.usage, RCBufferUsage::TransferDst) ||
                                   HasUsage(desc.usage, RCBufferUsage::TransferSrc);
        mStagingDirection = needsReadback ? StagingDirection::Readback
                                          : StagingDirection::WriteThrough;

        // 回读用 READBACK 堆（CPU 可读）；写回用 UPLOAD 堆（CPU 可写）
        mStagingHeapType = needsReadback ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD;
        const D3D12_RESOURCE_STATES stagingState =
            needsReadback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;

        const D3D12_RESOURCE_DESC stagingDesc = DX12BufferResourceDesc(allocSize, D3D12_RESOURCE_FLAG_NONE);
        hr = mContext->CreateResource(stagingDesc, mStagingHeapType, stagingState,
                                      &mStagingAllocation, IID_PPV_ARGS(&mStagingResource),
                                      nullptr, nullptr);
        if (FAILED(hr) || mStagingResource == nullptr)
        {
            LOG_ERROR("[DX12] Failed to create staging buffer for a DEFAULT-heap shared buffer: %s",
                      DX12HResultToString(hr));
            return false;
        }

        const D3D12_RANGE readRange = DX12ReadRange(0, 0);
        void* mapped = nullptr;
        if (SUCCEEDED(mStagingResource->Map(0, &readRange, &mapped)))
        {
            mStagingMapped = mapped;
        }
    }

    // ---- 上传初始数据 ----
    if (initialData != nullptr && desc.size > 0)
    {
        // DEFAULT heap 不能直接 Map。StorageModePrivate 的顶点/索引缓冲也不会
        // 保留持久 staging，因此需要在创建时用一个短命 UPLOAD 资源完成首次上传。
        // 之前这条路径直接调用 GetCpuPointer()，对 Private/DEFAULT 必然返回
        // nullptr，结果是资源创建成功但内容全为 0，所有几何静默消失。
        if (heapType == D3D12_HEAP_TYPE_DEFAULT && mStagingResource == nullptr)
        {
            D3D12MA::Allocation* uploadAllocation = nullptr;
            ComPtr<ID3D12Resource> uploadResource;
            const D3D12_RESOURCE_DESC uploadDesc =
                DX12BufferResourceDesc(allocSize, D3D12_RESOURCE_FLAG_NONE);
            hr = mContext->CreateResource(uploadDesc, D3D12_HEAP_TYPE_UPLOAD,
                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                          &uploadAllocation, IID_PPV_ARGS(&uploadResource),
                                          nullptr, nullptr);

            void* uploadMapped = nullptr;
            const D3D12_RANGE noRead = DX12ReadRange(0, 0);
            if (SUCCEEDED(hr) && uploadResource != nullptr &&
                SUCCEEDED(uploadResource->Map(0, &noRead, &uploadMapped)) &&
                uploadMapped != nullptr)
            {
                memcpy(uploadMapped, initialData, desc.size);
                const D3D12_RANGE writtenRange = DX12ReadRange(0, desc.size);
                uploadResource->Unmap(0, &writtenRange);

                DX12ExecuteOneShotCommandList(*mContext, [&](ID3D12GraphicsCommandList* cmdList) {
                    const D3D12_RESOURCE_BARRIER toDest =
                        DX12TransitionBarrier(mResource.Get(), mCurrentState,
                                             D3D12_RESOURCE_STATE_COPY_DEST);
                    cmdList->ResourceBarrier(1, &toDest);
                    cmdList->CopyBufferRegion(mResource.Get(), 0, uploadResource.Get(), 0, desc.size);
                    const D3D12_RESOURCE_BARRIER fromDest =
                        DX12TransitionBarrier(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                             mCurrentState);
                    cmdList->ResourceBarrier(1, &fromDest);
                });
                mNeedsUpload = false;
            }
            else
            {
                LOG_ERROR("[DX12] Failed to create/map upload buffer for initial data: %s",
                          DX12HResultToString(hr));
            }

            uploadResource.Reset();
            if (uploadAllocation != nullptr)
            {
                mContext->ReleaseAllocation(uploadAllocation);
            }
        }
        else
        {
            void* dst = GetCpuPointer();
            if (dst != nullptr)
            {
                memcpy(dst, initialData, desc.size);

                // DEFAULT 堆需要显式同步；UPLOAD 堆直接可见
                if (heapType == D3D12_HEAP_TYPE_DEFAULT)
                {
                    if (mStagingResource != nullptr && mStagingDirection == StagingDirection::WriteThrough)
                    {
                        SyncStagingToResource();
                    }
                }
                else
                {
                    mNeedsUpload = false;
                }
            }
            else
            {
                LOG_ERROR("[DX12] Failed to obtain CPU pointer for initial buffer upload");
            }
        }
    }

    return true;
}

void* DX12BufferBase::GetCpuPointer() const
{
    if (mPersistentMapped != nullptr)
    {
        return mPersistentMapped;
    }
    return mStagingMapped;
}

void DX12BufferBase::SetDebugName(const char* name)
{
    if (name == nullptr)
    {
        return;
    }
    mDebugName = name;
    if (mResource)
    {
        std::wstring wide(name, name + strlen(name));
        mResource->SetName(wide.c_str());
    }
}

// ============================================================================
// 暂存缓冲与 GPU 资源之间的同步
// ============================================================================

void DX12BufferBase::SyncStagingToResource()
{
    if (mContext == nullptr || mResource == nullptr || mStagingResource == nullptr || mSize == 0)
    {
        return;
    }

    if (mStagingDirection != StagingDirection::WriteThrough)
    {
        return;
    }

    const D3D12_RESOURCE_STATES previousState = mCurrentState;
    const bool needTransition = (previousState != D3D12_RESOURCE_STATE_COPY_DEST);

    DX12ExecuteOneShotCommandList(*mContext, [&](ID3D12GraphicsCommandList* cmdList) {
        if (needTransition)
        {
            const D3D12_RESOURCE_BARRIER toDest =
                DX12TransitionBarrier(mResource.Get(), previousState, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->ResourceBarrier(1, &toDest);
        }

        cmdList->CopyBufferRegion(mResource.Get(), 0, mStagingResource.Get(), 0, mSize);

        if (needTransition)
        {
            const D3D12_RESOURCE_BARRIER fromDest =
                DX12TransitionBarrier(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, previousState);
            cmdList->ResourceBarrier(1, &fromDest);
        }
    });

    mNeedsUpload = false;
}

void DX12BufferBase::SyncResourceToStaging()
{
    if (mContext == nullptr || mResource == nullptr || mStagingResource == nullptr || mSize == 0)
    {
        return;
    }

    if (mStagingDirection != StagingDirection::Readback)
    {
        return;
    }

    const D3D12_RESOURCE_STATES previousState = mCurrentState;
    const bool needTransition = (previousState != D3D12_RESOURCE_STATE_COPY_SOURCE);

    DX12ExecuteOneShotCommandList(*mContext, [&](ID3D12GraphicsCommandList* cmdList) {
        if (needTransition)
        {
            const D3D12_RESOURCE_BARRIER toSrc =
                DX12TransitionBarrier(mResource.Get(), previousState, D3D12_RESOURCE_STATE_COPY_SOURCE);
            cmdList->ResourceBarrier(1, &toSrc);
        }

        cmdList->CopyBufferRegion(mStagingResource.Get(), 0, mResource.Get(), 0, mSize);

        if (needTransition)
        {
            const D3D12_RESOURCE_BARRIER fromSrc =
                DX12TransitionBarrier(mResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, previousState);
            cmdList->ResourceBarrier(1, &fromSrc);
        }
    });

    mNeedsUpload = false;
}

// ============================================================================
// RCBuffer
// ============================================================================

DX12RCBuffer::DX12RCBuffer(const DX12ContextPtr& context, const RCBufferDesc& desc)
{
    CreateBufferInternal(context, desc, nullptr);
}

DX12RCBuffer::DX12RCBuffer(const DX12ContextPtr& context, const RCBufferDesc& desc, const void* data)
{
    CreateBufferInternal(context, desc, data);
}

bool DX12RCBuffer::SupportsUAV() const
{
    return IsResourceValid() && HasUsage(mUsage, RCBufferUsage::StorageBuffer);
}

void* DX12RCBuffer::Map() const
{
    DX12RCBuffer* self = const_cast<DX12RCBuffer*>(this);

    // 回读场景：先把 GPU 结果同步到 READBACK 暂存，CPU 才能读到有效数据
    if (mStagingResource != nullptr && mStagingDirection == StagingDirection::Readback)
    {
        self->SyncResourceToStaging();
    }

    return GetCpuPointer();
}

void DX12RCBuffer::Unmap() const
{
    // 写回场景：DEFAULT 堆的共享缓冲，CPU 改动落在暂存里，需要同步到 GPU 资源。
    // 这类缓冲（地形 patch 数据、间接参数清零）每帧调用次数极少，
    // 一次同步提交的开销可以接受，换来的是编码路径无需插入隐式拷贝的复杂度。
    if (mStagingResource != nullptr && mStagingDirection == StagingDirection::WriteThrough)
    {
        DX12RCBuffer* self = const_cast<DX12RCBuffer*>(this);
        self->mNeedsUpload = true;
        self->SyncStagingToResource();
    }
}

DXGI_FORMAT DX12RCBuffer::ToDXGIIndexFormat(IndexType indexType)
{
    return (indexType == IndexType_UInt) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
}

// ============================================================================
// UniformBuffer
// ============================================================================

DX12UniformBuffer::DX12UniformBuffer(const DX12ContextPtr& context, uint32_t size)
{
    // CBV 的大小必须是 256 字节对齐；同时为部分更新留出对齐余量。
    mAlignedSize = (uint32_t)DX12Util::AlignUp(size, GNX_DX12_CONSTANT_BUFFER_ALIGNMENT);

    RCBufferDesc desc(mAlignedSize, RCBufferUsage::UniformBuffer, StorageModeShared);
    CreateBufferInternal(context, desc, nullptr);

    mShadowData.resize(mAlignedSize, 0);
}

void DX12UniformBuffer::SetData(const void* data, uint32_t offset, uint32_t dataSize)
{
    if (data == nullptr || dataSize == 0)
    {
        return;
    }

    if ((uint64_t)offset + dataSize > mAlignedSize)
    {
        LOG_ERROR("[DX12] UniformBuffer::SetData out of range (offset=%u, size=%u, capacity=%u)",
                  offset, dataSize, mAlignedSize);
        return;
    }

    // 影子副本（与 Vulkan 后端语义一致，便于排查）
    memcpy(mShadowData.data() + offset, data, dataSize);

    void* dst = GetCpuPointer();
    if (dst != nullptr)
    {
        memcpy((uint8_t*)dst + offset, data, dataSize);
    }

    if (dst == nullptr)
    {
        LOG_ERROR("[DX12] UniformBuffer::SetData: CPU pointer is null (size=%u)", dataSize);
    }
}

NAMESPACE_RENDERCORE_END
