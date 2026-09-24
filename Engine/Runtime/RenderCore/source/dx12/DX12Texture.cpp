//
//  DX12Texture.cpp
//  rendercore
//

#include "DX12Texture.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

#include <mutex>

NAMESPACE_RENDERCORE_BEGIN

namespace
{
/// 依据资源标志推断“自然”初始状态（保证首次 barrier 的 StateBefore 合法）
D3D12_RESOURCE_STATES GetNaturalInitialState(D3D12_RESOURCE_FLAGS flags)
{
    if ((flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
    {
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if ((flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
    {
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

/// 全局递增的临时缓冲调试名序号
std::atomic<uint32_t> g_textureUploadCounter{0};

void WaitForCopyFence(ID3D12Fence* fence)
{
    if (!fence || fence->GetCompletedValue() >= 1)
        return;

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    bool waited = false;
    if (event)
    {
        if (SUCCEEDED(fence->SetEventOnCompletion(1, event)))
            waited = WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0;
        CloseHandle(event);
    }
    if (!waited)
    {
        // Event creation/registration can fail under resource pressure.
        // Never release an in-flight upload heap in that case.
        UINT64 value;
        do {
            Sleep(1);
            value = fence->GetCompletedValue();
        } while (value < 1 && value != UINT64_MAX);
    }
}

class DX12TextureUpload final : public TextureUpload
{
public:
    DX12TextureUpload(DX12ContextPtr context, DX12RCTexture2DPtr texture,
                      D3D12MA::Allocation* stagingAllocation,
                      ComPtr<ID3D12Resource> staging,
                      ComPtr<ID3D12CommandAllocator> commandAllocator,
                      ComPtr<ID3D12GraphicsCommandList> commandList,
                      ComPtr<ID3D12Fence> fence)
        : mContext(std::move(context)), mTexture(std::move(texture)),
          mStagingAllocation(stagingAllocation), mStaging(std::move(staging)),
          mCommandAllocator(std::move(commandAllocator)),
          mCommandList(std::move(commandList)), mFence(std::move(fence)) {}

    ~DX12TextureUpload() override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // Keep the upload heap and destination texture alive until the copy
        // queue has completed, including when a tile is evicted early.
        WaitForCopyFence(mFence.Get());
        ReleaseResources();
    }

    TextureUploadStatus GetStatus() const override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mStatus != TextureUploadStatus::Pending)
            return mStatus;
        const UINT64 value = mFence->GetCompletedValue();
        if (value == UINT64_MAX)
            mStatus = TextureUploadStatus::Failed;
        else if (value >= 1)
            mStatus = TextureUploadStatus::Complete;
        if (mStatus != TextureUploadStatus::Pending)
        {
            ReleaseResources();
            if (mStatus == TextureUploadStatus::Complete)
            {
                static std::atomic<bool> loggedRelease{false};
                if (!loggedRelease.exchange(true))
                    LOG_INFO("[DX12] Completed texture upload staging and fence released");
            }
        }
        return mStatus;
    }

private:
    void ReleaseResources() const
    {
        mCommandList.Reset();
        mCommandAllocator.Reset();
        mStaging.Reset();
        if (mStagingAllocation)
        {
            mContext->ReleaseAllocation(mStagingAllocation);
            mStagingAllocation = nullptr;
        }
        mFence.Reset();
        mTexture.reset();
        mContext.reset();
    }

    mutable std::mutex mMutex;
    mutable TextureUploadStatus mStatus = TextureUploadStatus::Pending;
    mutable DX12ContextPtr mContext;
    mutable DX12RCTexture2DPtr mTexture;
    mutable D3D12MA::Allocation* mStagingAllocation;
    mutable ComPtr<ID3D12Resource> mStaging;
    mutable ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    mutable ComPtr<ID3D12GraphicsCommandList> mCommandList;
    mutable ComPtr<ID3D12Fence> mFence;
};
} // namespace

// ============================================================================
// 构造 / 析构
// ============================================================================

DX12TextureBase::DX12TextureBase(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                                 TextureFormat format)
    // RCTexture 是虚基类且没有默认构造函数，中间类也必须显式初始化它
    // （真正的类型由最派生的 DX12RCTexture* 指定，这里的初始化在那种情况下会被忽略）
    : RCTexture(TextureType_Unkown)
    , mContext(context)
{
    if (!mContext || !mContext->IsValid())
    {
        LOG_ERROR("[DX12] DX12TextureBase created with an invalid context");
        return;
    }

    mFormat = desc.Format;
    mWidth  = (uint32_t)desc.Width;
    mHeight = desc.Height;
    mDepth  = 1;
    mMipLevels = desc.MipLevels;
    mLayerCount = desc.DepthOrArraySize;
    mResourceDimension = desc.Dimension;
    mIsDepthStencil = DX12Util::IsDepthStencilFormat(mFormat) ||
                      DX12Util::IsDepthStencilFormat(DX12Util::GetTypelessFormat(mFormat));

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        mDepth = desc.DepthOrArraySize;
        mLayerCount = 1;
    }

    // 深度格式必须以 typeless 创建，否则资源无法同时作为 DSV 与 SRV 使用
    mTypelessFormat = mIsDepthStencil ? DX12Util::GetTypelessFormat(mFormat) : mFormat;

    D3D12_RESOURCE_DESC createDesc = desc;
    createDesc.Format = mTypelessFormat;

    if (!CreateResource(createDesc))
    {
        return;
    }

    SetFormat(format);
    SetNaturalState(GetNaturalInitialState(desc.Flags));
}

DX12TextureBase::DX12TextureBase(const DX12ContextPtr& context, ID3D12Resource* externalResource,
                                 DXGI_FORMAT viewFormat, uint32_t width, uint32_t height,
                                 D3D12_RESOURCE_STATES currentState)
    : RCTexture(TextureType_Unkown)
    , mContext(context)
{
    if (!mContext || externalResource == nullptr)
    {
        LOG_ERROR("[DX12] DX12TextureBase wrapping a null external resource");
        return;
    }

    mResource = externalResource;   // 延迟 AddRef（ComPtr 赋值会 AddRef）
    mFormat = viewFormat;
    mTypelessFormat = viewFormat;
    mWidth = width;
    mHeight = height;
    mDepth = 1;
    mMipLevels = 1;
    mLayerCount = 1;
    mResourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    mIsDepthStencil = false;
    mAllocation = nullptr;          // 外部资源不由本对象释放
    mIsExternal = true;

    SetNaturalState(currentState);
}

DX12TextureBase::~DX12TextureBase()
{
    // 外部资源（交换链 back buffer）由 DXGI 拥有，不在此释放
    if (mIsExternal)
    {
        mResource.Reset();
        return;
    }

    mResource.Reset();

    if (mAllocation != nullptr && mContext != nullptr && mContext->allocator != nullptr)
    {
        mContext->ReleaseAllocation(mAllocation);
        mAllocation = nullptr;
    }
}

bool DX12TextureBase::CreateResource(const D3D12_RESOURCE_DESC& desc)
{
    const D3D12_RESOURCE_STATES initialState = GetNaturalInitialState(desc.Flags);

    // RT / DS 资源必须提供优化清除值：否则每次 ClearRenderTargetView 的值与
    // 创建时的默认值不一致，调试层会刷 MISMATCHINGCLEARVALUE，且部分驱动会
    // 触发额外的解压缩开销。
    D3D12_CLEAR_VALUE clearValue = {};
    const D3D12_CLEAR_VALUE* pClearValue = nullptr;
    if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
    {
        clearValue.Format = mFormat;
        clearValue.DepthStencil.Depth = DepthConfig::UseReverseZ ? 0.0f : 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
    }
    else if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
    {
        clearValue.Format = mFormat;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 0.0f;
        pClearValue = &clearValue;
    }

    const HRESULT hr = mContext->CreateResource(desc, D3D12_HEAP_TYPE_DEFAULT, initialState,
                                                &mAllocation, IID_PPV_ARGS(&mResource),
                                                pClearValue, mDebugName.c_str());
    if (FAILED(hr) || mResource == nullptr)
    {
        LOG_ERROR("[DX12] Failed to create texture resource (%llux%u, mips=%u, layers=%u, format=%d): %s",
                  (unsigned long long)desc.Width, desc.Height, desc.MipLevels,
                  desc.DepthOrArraySize, (int)desc.Format, DX12HResultToString(hr));
        if (mAllocation != nullptr)
        {
            mContext->ReleaseAllocation(mAllocation);
            mAllocation = nullptr;
        }
        return false;
    }

    return true;
}

// ============================================================================
// 基础查询
// ============================================================================

bool DX12TextureBase::IsValid() const
{
    return mResource != nullptr;
}

void DX12TextureBase::SetName(const char* name)
{
    if (name == nullptr)
    {
        return;
    }
    mDebugName = name;
    if (mResource)
    {
        std::wstring wideName(name, name + strlen(name));
        mResource->SetName(wideName.c_str());
    }
}

D3D12_GPU_VIRTUAL_ADDRESS DX12TextureBase::GetGPUAddress() const
{
    return mResource ? mResource->GetGPUVirtualAddress() : 0;
}

uint64_t DX12TextureBase::GetSizeInBytes() const
{
    if (!mResource)
    {
        return 0;
    }
    // D3D12MA 提供精确的分配大小；没有分配对象时退回资源描述中的尺寸估算
    return mResource->GetDesc().Width;
}

void DX12TextureBase::GetMipSize(uint32_t mip, uint32_t& w, uint32_t& h) const
{
    DX12Util::GetMipDimensions(mWidth, mHeight, mip, w, h);
}

D3D12_CLEAR_VALUE DX12TextureBase::GetOptimizedClearValue(bool depth) const
{
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = mFormat;
    if (depth)
    {
        clearValue.DepthStencil.Depth = DepthConfig::UseReverseZ ? 0.0f : 1.0f;
        clearValue.DepthStencil.Stencil = 0;
    }
    return clearValue;
}

// ============================================================================
// 视图写入
// ============================================================================

void DX12TextureBase::WriteSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest,
                               uint32_t mipLevel, uint32_t firstSlice, uint32_t sliceCount)
{
    if (device == nullptr || mResource == nullptr)
    {
        return;
    }

    // 计算视图范围
    const uint32_t startMip = (mipLevel == DX12_ALL_MIPS) ? 0u : mipLevel;
    if (startMip >= mMipLevels)
    {
        LOG_ERROR("[DX12] WriteSRV: mip %u out of range (mipLevels=%u)", startMip, mMipLevels);
        return;
    }
    const uint32_t viewMips = (mipLevel == DX12_ALL_MIPS) ? (mMipLevels - startMip) : 1u;

    const uint32_t startSlice = firstSlice;
    const uint32_t totalSlices = (mIsCube ? 6u : mLayerCount);
    const uint32_t viewSlices = (sliceCount == DX12_ALL_SLICES) ? (totalSlices - startSlice) : sliceCount;

    if (startSlice + viewSlices > totalSlices)
    {
        LOG_ERROR("[DX12] WriteSRV: slice range [%u, %u) exceeds layer count %u",
                  startSlice, startSlice + viewSlices, totalSlices);
        return;
    }

    // 深度纹理的 SRV 必须使用 R* 格式
    const DXGI_FORMAT srvFormat = mIsDepthStencil ? DX12Util::GetDepthSRVFormat(mFormat) : mFormat;

    D3D12_SRV_DIMENSION dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    if (mResourceDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        dimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    }
    else if (mIsCube && viewSlices == 6 && startSlice == 0)
    {
        dimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    }
    else if (viewSlices > 1 || (mIsCube && viewSlices != 6))
    {
        dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    }
    else if (mResourceDimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && viewSlices == 1)
    {
        dimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    }

    const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12TextureSRVDesc(
        srvFormat, dimension, viewMips, startMip, startSlice, viewSlices);

    device->CreateShaderResourceView(mResource.Get(), &srvDesc, dest);
}

void DX12TextureBase::WriteUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest,
                               uint32_t mipLevel)
{
    if (device == nullptr || mResource == nullptr)
    {
        return;
    }

    if (DX12Util::IsSRGBFormat(mFormat))
    {
        // D3D12 不允许 sRGB UAV：静默失败会导致 shader 读到未初始化的描述符。
        // 这里明确报错，便于尽早定位资产/用法问题。
        LOG_ERROR("[DX12] WriteUAV on an sRGB texture (format=%d) is not allowed by D3D12",
                  (int)mFormat);
        return;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    if (mResourceDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        uavDesc = DX12TextureUAVDesc(mFormat, D3D12_UAV_DIMENSION_TEXTURE3D, mipLevel, 0, 1, mDepth);
    }
    else if (mLayerCount > 1 || mIsCube)
    {
        const uint32_t slices = mIsCube ? 6u : mLayerCount;
        uavDesc = DX12TextureUAVDesc(mFormat, D3D12_UAV_DIMENSION_TEXTURE2DARRAY, mipLevel, 0, slices);
    }
    else
    {
        uavDesc = DX12TextureUAVDesc(mFormat, D3D12_UAV_DIMENSION_TEXTURE2D, mipLevel);
    }

    device->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, dest);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureBase::AllocateRTV(uint32_t mipLevel, uint32_t slice)
{
    const uint32_t key = mipLevel * 1024u + slice;
    auto iter = mRTVCache.find(key);
    if (iter != mRTVCache.end())
    {
        return iter->second;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    if (mContext == nullptr || !mContext->rtvPool || !mContext->rtvPool->IsValid())
    {
        LOG_ERROR("[DX12] RTV pool unavailable, cannot create RTV for texture");
        return handle;
    }

    uint32_t index = 0;
    if (!mContext->rtvPool->Allocate(1, index))
    {
        LOG_ERROR("[DX12] RTV pool exhausted");
        return handle;
    }

    handle = mContext->rtvPool->GetCpuHandle(index);

    // 视图维度必须与资源类型匹配，否则调试层报
    //   id=42 "The ViewDimension in the View Desc is incompatible with the type of the Resource"
    //   id=41 "MipSlice/FirstWSlice ... must be between ..."
    // 3D 纹理（如 atmosphere 的 LUT）必须用 TEXTURE3D 视图：
    // 此时切片是 W 方向，需要写 Texture3D.FirstWSlice / WSize。
    const D3D12_RESOURCE_DESC resourceDesc = mResource->GetDesc();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        rtvDesc.Format               = mFormat;
        rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice   = mipLevel;
        rtvDesc.Texture3D.FirstWSlice = slice;
        rtvDesc.Texture3D.WSize      = 1;
    }
    else
    {
        const uint32_t slices = mIsCube ? 6u : mLayerCount;
        const D3D12_RTV_DIMENSION dimension =
            (slices > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc = DX12RTVDesc(mFormat, dimension, mipLevel, slice, 1);
    }

    mContext->device->CreateRenderTargetView(mResource.Get(), &rtvDesc, handle);
    mRTVCache.emplace(key, handle);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureBase::GetRTVHandle(uint32_t mipLevel, uint32_t slice)
{
    if (mResource == nullptr)
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }
    if (mIsDepthStencil)
    {
        LOG_ERROR("[DX12] GetRTVHandle called on a depth-stencil texture");
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }
    return AllocateRTV(mipLevel, slice);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureBase::AllocateDSV()
{
    if (mDSVCreated)
    {
        return mDSVHandle;
    }

    if (!mIsDepthStencil || mResource == nullptr)
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }

    if (mContext == nullptr || !mContext->dsvPool || !mContext->dsvPool->IsValid())
    {
        LOG_ERROR("[DX12] DSV pool unavailable, cannot create DSV for texture");
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }

    uint32_t index = 0;
    if (!mContext->dsvPool->Allocate(1, index))
    {
        LOG_ERROR("[DX12] DSV pool exhausted");
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }

    mDSVHandle = mContext->dsvPool->GetCpuHandle(index);

    const uint32_t slices = mLayerCount;
    const D3D12_DSV_DIMENSION dimension =
        (slices > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
    const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = DX12DSVDesc(mFormat, dimension, 0, 0, slices);

    mContext->device->CreateDepthStencilView(mResource.Get(), &dsvDesc, mDSVHandle);
    mDSVCreated = true;
    return mDSVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureBase::GetDSVHandle()
{
    return AllocateDSV();
}

// ============================================================================
// 数据上传
// ============================================================================

void DX12TextureBase::ReplaceRegion(const Rect2D& rect, uint32_t level, uint32_t slice,
                                    const uint8_t* pixelBytes, uint32_t bytesPerRow,
                                    uint32_t bytesPerImage)
{
    (void)bytesPerImage;

    if (mResource == nullptr || pixelBytes == nullptr || mContext == nullptr)
    {
        return;
    }

    if (level >= mMipLevels)
    {
        LOG_ERROR("[DX12] ReplaceRegion: level %u out of range (%u)", level, mMipLevels);
        return;
    }

    const uint32_t totalSlices = (mResourceDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
                                     ? mDepth : mLayerCount;
    if (slice >= totalSlices)
    {
        LOG_ERROR("[DX12] ReplaceRegion: slice %u out of range (%u)", slice, totalSlices);
        return;
    }

    // ---- 计算拷贝区域（rect 为 0 时按整张 mip 处理）----
    uint32_t mipWidth = 0, mipHeight = 0;
    GetMipSize(level, mipWidth, mipHeight);

    uint32_t copyWidth = (rect.width > 0) ? (uint32_t)rect.width : mipWidth;
    uint32_t copyHeight = (rect.height > 0) ? (uint32_t)rect.height : mipHeight;
    const uint32_t offsetX = (uint32_t)((rect.offsetX > 0) ? rect.offsetX : 0);
    const uint32_t offsetY = (uint32_t)((rect.offsetY > 0) ? rect.offsetY : 0);

    if (offsetX + copyWidth > mipWidth)   copyWidth  = mipWidth - offsetX;
    if (offsetY + copyHeight > mipHeight) copyHeight = mipHeight - offsetY;

    if (copyWidth == 0 || copyHeight == 0)
    {
        return;
    }

    // ---- 用 GetCopyableFootprints 取得 D3D12 要求的行距/总字节数 ----
    const uint32_t subresource = GetSubresourceIndex(level, slice);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    UINT64 totalBytes = 0;

    D3D12_RESOURCE_DESC resourceDesc = mResource->GetDesc();
    mContext->device->GetCopyableFootprints(&resourceDesc, subresource, 1, 0,
                                            &footprint, &numRows, &rowSizeInBytes, &totalBytes);
    if (totalBytes == 0)
    {
        LOG_ERROR("[DX12] ReplaceRegion: GetCopyableFootprints returned 0 bytes");
        return;
    }

    // ---- 创建暂存（UPLOAD）缓冲 ----
    D3D12MA::Allocation* stagingAllocation = nullptr;
    ComPtr<ID3D12Resource> staging;
    const D3D12_RESOURCE_DESC stagingDesc = DX12BufferResourceDesc(totalBytes, D3D12_RESOURCE_FLAG_NONE);
    const HRESULT hr = mContext->CreateResource(stagingDesc, D3D12_HEAP_TYPE_UPLOAD,
                                                D3D12_RESOURCE_STATE_GENERIC_READ, &stagingAllocation,
                                                IID_PPV_ARGS(&staging), nullptr, nullptr);
    if (FAILED(hr) || staging == nullptr)
    {
        LOG_ERROR("[DX12] ReplaceRegion: failed to create staging buffer: %s",
                  DX12HResultToString(hr));
        return;
    }

    // ---- 按行重打包（源行距由调用方给出，目标行距由 D3D12 决定）----
    {
        void* mapped = nullptr;
        const D3D12_RANGE readRange = DX12ReadRange(0, 0);
        if (SUCCEEDED(staging->Map(0, &readRange, &mapped)) && mapped != nullptr)
        {
            uint8_t* dst = (uint8_t*)mapped + footprint.Offset;

            // 块压缩格式：源行数按块行计算
            const TextureBlockInfo blockInfo = GetCompressedTextureBlockInfo(GetTextureFormat());
            const uint32_t blockWidth  = (blockInfo.bytesPerBlock > 0) ? blockInfo.blockWidth : 1u;
            const uint32_t blockHeight = (blockInfo.bytesPerBlock > 0) ? blockInfo.blockHeight : 1u;

            const uint32_t pixelRowsToCopy  = (copyHeight + blockHeight - 1) / blockHeight;
            // 非块压缩格式的每行字节数按"每像素字节数 × 像素数"计算。
            //
            // Derive texel size from DXGI; the generic helper rejects some float formats.
            const uint32_t bytesPerTexel = (DX12Util::GetFormatBitsPerPixel(GetTextureFormat()) + 7u) / 8u;

            const uint32_t bytesPerCopyRow  = (uint32_t)((copyWidth + blockWidth - 1) / blockWidth *
                                                         ((blockInfo.bytesPerBlock > 0)
                                                              ? blockInfo.bytesPerBlock
                                                              : bytesPerTexel));

            const uint32_t srcRowPitch = (bytesPerRow > 0) ? bytesPerRow : bytesPerCopyRow;

            for (uint32_t row = 0; row < pixelRowsToCopy; ++row)
            {
                const uint8_t* srcRow = pixelBytes + (size_t)row * srcRowPitch;
                uint8_t* dstRow = dst + (size_t)row * footprint.Footprint.RowPitch;
                memcpy(dstRow, srcRow, bytesPerCopyRow);
            }

            const D3D12_RANGE writtenRange = DX12ReadRange(0, totalBytes);
            staging->Unmap(0, &writtenRange);
        }
        else
        {
            LOG_ERROR("[DX12] ReplaceRegion: failed to map staging buffer");
            mContext->ReleaseAllocation(stagingAllocation);
            return;
        }
    }

    // ---- 一次性命令列表：barrier → CopyTextureRegion → barrier 恢复 ----
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    if (FAILED(mContext->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                        IID_PPV_ARGS(&allocator))) ||
        FAILED(mContext->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                   allocator.Get(), nullptr,
                                                   IID_PPV_ARGS(&commandList))))
    {
        LOG_ERROR("[DX12] ReplaceRegion: failed to create a one-shot command list");
        mContext->ReleaseAllocation(stagingAllocation);
        return;
    }

    const D3D12_RESOURCE_STATES previousState = mCurrentState;

    if (previousState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        const D3D12_RESOURCE_BARRIER toCopyDest =
            DX12TransitionBarrier(mResource.Get(), previousState, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &toCopyDest);
    }

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource        = mResource.Get();
    dstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = subresource;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource       = staging.Get();
    srcLocation.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_BOX srcBox = {};
    srcBox.left   = 0;
    srcBox.top    = 0;
    srcBox.front  = 0;
    srcBox.right  = copyWidth;
    srcBox.bottom = copyHeight;
    srcBox.back   = 1;

    // 块压缩格式（BC1-7）要求 CopyTextureRegion 的 pSrcBox 按块边界（4×4）对齐，
    // 否则调试层报错（"coordinates in pSrcBox are not aligned properly"）并可能
    // 导致设备移除。例如 BC6H 的 2×2 mip 必须写成 0..4 而不是 0..2。
    const TextureBlockInfo copyBlockInfo = GetCompressedTextureBlockInfo(GetTextureFormat());
    if (copyBlockInfo.bytesPerBlock > 0)
    {
        const uint32_t blockWidth  = (copyBlockInfo.blockWidth  > 0) ? copyBlockInfo.blockWidth  : 4u;
        const uint32_t blockHeight = (copyBlockInfo.blockHeight > 0) ? copyBlockInfo.blockHeight : 4u;
        srcBox.left   = (srcBox.left   / blockWidth)  * blockWidth;
        srcBox.top    = (srcBox.top    / blockHeight) * blockHeight;
        srcBox.right  = ((srcBox.right  + blockWidth  - 1) / blockWidth)  * blockWidth;
        srcBox.bottom = ((srcBox.bottom + blockHeight - 1) / blockHeight) * blockHeight;
    }

    commandList->CopyTextureRegion(&dstLocation, offsetX, offsetY, 0, &srcLocation, &srcBox);

    if (previousState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        const D3D12_RESOURCE_BARRIER fromCopyDest =
            DX12TransitionBarrier(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, previousState);
        commandList->ResourceBarrier(1, &fromCopyDest);
    }

    commandList->Close();

    ID3D12CommandList* lists[] = { commandList.Get() };
    mContext->graphicsQueue->ExecuteCommandLists(1, lists);
    mContext->graphicsFence.Signal(mContext->graphicsQueue.Get());

    // 阻塞等待上传完成：ReplaceRegion 仅用于资产加载阶段，语义与
    // Vulkan 后端的 BeginSingleTimeCommand/EndSingleTimeCommand 一致。
    if (!mContext->graphicsFence.WaitForIdle(10000))
    {
        LOG_ERROR("[DX12] ReplaceRegion: GPU wait timed out");
    }

    mCurrentState = previousState;

    mContext->ReleaseAllocation(stagingAllocation);
}

TextureUploadPtr DX12RCTexture2D::ReplaceRegionAsync(const Rect2D& rect, uint32_t level,
                                                      const uint8_t* pixels, uint32_t bytesPerRow)
{
    auto failed = [] { return std::make_shared<FailedTextureUpload>(); };
    const auto context = GetDX12Context();
    ID3D12Resource* texture = GetResource();
    if (!context || !texture || !pixels || rect.width <= 0 || rect.height <= 0 ||
        level >= GetMipLevels())
    {
        LOG_ERROR("[DX12] Async upload precondition: context=%d copyQueue=%d texture=%d pixels=%d size=%dx%d level=%u/%u state=%u",
                  context != nullptr, context && context->copyQueue != nullptr,
                  texture != nullptr, pixels != nullptr, rect.width, rect.height,
                  level, GetMipLevels(), static_cast<unsigned>(GetCurrentState()));
        return failed();
    }
    if (!context->copyQueue || GetCurrentState() != D3D12_RESOURCE_STATE_COMMON)
    {
        ReplaceRegion(rect, level, pixels, bytesPerRow);
        return std::make_shared<CompletedTextureUpload>();
    }

    const uint32_t width = static_cast<uint32_t>(rect.width);
    const uint32_t height = static_cast<uint32_t>(rect.height);
    if (rect.offsetX < 0 || rect.offsetY < 0 ||
        static_cast<uint32_t>(rect.offsetX) + width > std::max(1u, GetWidth() >> level) ||
        static_cast<uint32_t>(rect.offsetY) + height > std::max(1u, GetHeight() >> level))
        return failed();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT numRows = 0;
    UINT64 rowSize = 0, totalBytes = 0;
    const D3D12_RESOURCE_DESC desc = texture->GetDesc();
    context->device->GetCopyableFootprints(&desc, level, 1, 0,
                                           &footprint, &numRows, &rowSize, &totalBytes);
    if (!totalBytes) { LOG_ERROR("[DX12] Async upload: no footprint"); return failed(); }
    D3D12MA::Allocation* stagingAllocation = nullptr;
    ComPtr<ID3D12Resource> staging;
    const HRESULT createResult = context->CreateResource(
        DX12BufferResourceDesc(totalBytes, D3D12_RESOURCE_FLAG_NONE),
        D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ,
        &stagingAllocation, IID_PPV_ARGS(&staging), nullptr, nullptr);
    if (FAILED(createResult) || !staging)
    {
        LOG_ERROR("[DX12] Async upload: staging creation failed %s", DX12HResultToString(createResult));
        return failed();
    }
    auto releaseStaging = [&] { context->ReleaseAllocation(stagingAllocation); };

    const TextureBlockInfo block = GetCompressedTextureBlockInfo(GetTextureFormat());
    const uint32_t blockWidth = block.bytesPerBlock ? block.blockWidth : 1;
    const uint32_t blockHeight = block.bytesPerBlock ? block.blockHeight : 1;
    const uint32_t bytesPerPixel = (DX12Util::GetFormatBitsPerPixel(GetTextureFormat()) + 7u) / 8u;
    const uint32_t copyRowBytes = ((width + blockWidth - 1) / blockWidth) *
                                  (block.bytesPerBlock ? block.bytesPerBlock : bytesPerPixel);
    const uint32_t rows = (height + blockHeight - 1) / blockHeight;
    if (!copyRowBytes || bytesPerRow < copyRowBytes || rows > numRows)
        { LOG_ERROR("[DX12] Async upload: row layout invalid source=%u copy=%u rows=%u/%u", bytesPerRow, copyRowBytes, rows, numRows); releaseStaging(); return failed(); }
    void* mapped = nullptr;
    const D3D12_RANGE readRange = DX12ReadRange(0, 0);
    if (FAILED(staging->Map(0, &readRange, &mapped)) || !mapped)
        { LOG_ERROR("[DX12] Async upload: staging map failed"); releaseStaging(); return failed(); }
    for (uint32_t row = 0; row < rows; ++row)
        memcpy(static_cast<uint8_t*>(mapped) + footprint.Offset +
                   static_cast<size_t>(row) * footprint.Footprint.RowPitch,
               pixels + static_cast<size_t>(row) * bytesPerRow, copyRowBytes);
    const D3D12_RANGE writtenRange = DX12ReadRange(0, totalBytes);
    staging->Unmap(0, &writtenRange);

    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;
    if (FAILED(context->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
                                                       IID_PPV_ARGS(&allocator))) ||
        FAILED(context->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY,
                                                   allocator.Get(), nullptr,
                                                   IID_PPV_ARGS(&list))) ||
        FAILED(context->device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                            IID_PPV_ARGS(&fence))))
        { LOG_ERROR("[DX12] Async upload: copy commands or fence creation failed"); releaseStaging(); return failed(); }

    // COPY queues use implicit COMMON -> COPY_DEST promotion. A legacy
    // ResourceBarrier here is invalid on a COPY command list (debug id 1334).
    D3D12_TEXTURE_COPY_LOCATION destination{};
    destination.pResource = texture;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destination.SubresourceIndex = level;
    D3D12_TEXTURE_COPY_LOCATION source{};
    source.pResource = staging.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint = footprint;
    D3D12_BOX box{};
    box.right = width;
    box.bottom = height;
    box.back = 1;
    if (block.bytesPerBlock)
    {
        box.right = ((width + blockWidth - 1) / blockWidth) * blockWidth;
        box.bottom = ((height + blockHeight - 1) / blockHeight) * blockHeight;
    }
    list->CopyTextureRegion(&destination, rect.offsetX, rect.offsetY, 0,
                            &source, &box);
    // The promoted state decays back to COMMON when the copy completes.
    if (FAILED(list->Close())) { LOG_ERROR("[DX12] Async upload: command list close failed"); releaseStaging(); return failed(); }
    ID3D12CommandList* lists[] = {list.Get()};
    context->copyQueue->ExecuteCommandLists(1, lists);
    if (FAILED(context->copyQueue->Signal(fence.Get(), 1)))
    {
        context->copyFence.FlushAndWait(context->copyQueue.Get(), 10000);
        releaseStaging();
        return failed();
    }
    // A GPU queue wait establishes ordering and visibility before any later
    // graphics submission that samples this texture.
    if (FAILED(context->graphicsQueue->Wait(fence.Get(), 1)))
    {
        WaitForCopyFence(fence.Get());
        releaseStaging();
        return failed();
    }
    static bool loggedCopyQueue = false;
    if (!loggedCopyQueue)
    {
        LOG_INFO("[DX12] Asynchronous texture upload submitted on Copy Queue");
        loggedCopyQueue = true;
    }
    return std::make_shared<DX12TextureUpload>(context, shared_from_this(),
        stagingAllocation, std::move(staging), std::move(allocator),
        std::move(list), std::move(fence));
}

// ============================================================================
// 外部资源包装
// ============================================================================

std::shared_ptr<DX12RCTexture2D> DX12WrapExternalTexture2D(const DX12ContextPtr& context,
                                                           ID3D12Resource* resource,
                                                           DXGI_FORMAT viewFormat,
                                                           uint32_t width, uint32_t height,
                                                           D3D12_RESOURCE_STATES currentState)
{
    if (context == nullptr || resource == nullptr)
    {
        return nullptr;
    }

    auto texture = std::make_shared<DX12RCTexture2D>(context, resource, viewFormat, width, height,
                                                     currentState);
    return texture;
}

NAMESPACE_RENDERCORE_END
