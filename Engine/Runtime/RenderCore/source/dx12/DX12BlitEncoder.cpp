//
//  DX12BlitEncoder.cpp
//  rendercore
//

#include "DX12BlitEncoder.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

NAMESPACE_RENDERCORE_BEGIN

namespace
{
/// D3D12 要求 buffer↔texture 拷贝的每行字节数按 256 对齐
constexpr uint64_t kTextureDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

bool IsRowPitchAligned(uint64_t bytesPerRow)
{
    return bytesPerRow != 0 && (bytesPerRow % kTextureDataPitchAlignment) == 0;
}

/**
 * @brief 把拷贝框对齐到块压缩格式的块边界
 *
 * D3D12 对 BC 系列格式要求 CopyTextureRegion 的 pSrcBox 必须按块对齐
 * （4×4），否则调试层报错并可能导致设备移除。例如 BC6H 的 2×2 mip：
 * 合法的框必须是 0..4 而不是 0..2（边缘不足一个块时向上取整由驱动处理）。
 */
void AlignBoxToBlockBoundary(TextureFormat format, D3D12_BOX& box)
{
    const TextureBlockInfo blockInfo = GetCompressedTextureBlockInfo(format);
    if (blockInfo.bytesPerBlock == 0)
    {
        return;   // 非块压缩格式无需对齐
    }

    const uint32_t blockWidth  = (blockInfo.blockWidth  > 0) ? blockInfo.blockWidth  : 4u;
    const uint32_t blockHeight = (blockInfo.blockHeight > 0) ? blockInfo.blockHeight : 4u;

    box.left   = (box.left   / blockWidth)  * blockWidth;
    box.top    = (box.top    / blockHeight) * blockHeight;
    box.right  = ((box.right  + blockWidth  - 1) / blockWidth)  * blockWidth;
    box.bottom = ((box.bottom + blockHeight - 1) / blockHeight) * blockHeight;
}
} // namespace

DX12BlitEncoder::DX12BlitEncoder(const DX12CommandBufferPtr& commandBuffer)
    : mCommandBuffer(commandBuffer)
{
    if (!mCommandBuffer || mCommandBuffer->GetCommandList() == nullptr)
    {
        return;
    }
    mCommandList = mCommandBuffer->GetCommandList();
    mContext = mCommandBuffer->GetContext().get();
    mEncoding = true;
}

DX12BlitEncoder::~DX12BlitEncoder()
{
    if (mEncoding)
    {
        EndEncode();
    }
}

void DX12BlitEncoder::EndEncode()
{
    mEncoding = false;
}

// ============================================================================
// Buffer 到 Buffer
// ============================================================================

void DX12BlitEncoder::CopyBuffer(RCBufferPtr source, uint64_t sourceOffset,
                                 RCBufferPtr destination, uint64_t destinationOffset,
                                 uint64_t size)
{
    auto src = std::dynamic_pointer_cast<DX12RCBuffer>(source);
    auto dst = std::dynamic_pointer_cast<DX12RCBuffer>(destination);
    if (!src || !dst || !mCommandList)
    {
        return;
    }

    // 拷贝两侧都需要处于 COPY_SRC / COPY_DST 状态（UPLOAD 堆为 GENERIC_READ，可同时作源）
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    if (src->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_SOURCE &&
        src->GetCurrentState() != D3D12_RESOURCE_STATE_GENERIC_READ)
    {
        barriers.push_back(DX12TransitionBarrier(src->GetResource(), src->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_SOURCE));
        src->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }

    if (dst->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST &&
        dst->GetCurrentState() != D3D12_RESOURCE_STATE_GENERIC_READ)
    {
        barriers.push_back(DX12TransitionBarrier(dst->GetResource(), dst->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_DEST));
        dst->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
    }

    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    mCommandList->CopyBufferRegion(dst->GetResource(), destinationOffset,
                                   src->GetResource(), sourceOffset, size);
}

void DX12BlitEncoder::FillBuffer(RCBufferPtr destination, uint64_t destinationOffset,
                                 const void* data, uint64_t dataSize)
{
    auto dst = std::dynamic_pointer_cast<DX12RCBuffer>(destination);
    if (!dst || !dst->IsValid() || !data || dataSize == 0 ||
        destinationOffset > dst->GetSizeInBytes() ||
        dataSize > (uint64_t)dst->GetSizeInBytes() - destinationOffset)
    {
        LOG_ERROR("[DX12] FillBuffer: invalid destination range or data");
        return;
    }

    RCBufferDesc stagingDesc((uint32_t)dataSize, RCBufferUsage::TransferSrc, StorageModeShared);
    auto staging = std::make_shared<DX12RCBuffer>(mCommandBuffer->GetContext(), stagingDesc, data);
    if (!staging->IsValid())
    {
        LOG_ERROR("[DX12] FillBuffer: failed to create upload buffer");
        return;
    }

    // CopyBufferRegion 只记录资源引用，不持有资源；由命令缓冲保活到帧槽位安全复用。
    mCommandBuffer->RetainTransientBuffer(staging);
    CopyBuffer(staging, 0, destination, destinationOffset, dataSize);
}

// ============================================================================
// Texture ↔ Buffer
// ============================================================================

void DX12BlitEncoder::DoBufferToTexture(ID3D12Resource* source, uint64_t sourceOffset,
                                        uint64_t sourceBytesPerRow,
                                        DX12TextureBase* destination, uint32_t slice, uint32_t mip,
                                        const mathutil::Vector2i& offset,
                                        const mathutil::Vector2i& size)
{
    if (source == nullptr || destination == nullptr || mCommandList == nullptr)
    {
        return;
    }

    if (!IsRowPitchAligned(sourceBytesPerRow))
    {
        LOG_ERROR("[DX12] CopyBufferToTexture: sourceBytesPerRow=%llu 未按 256 字节对齐，"
                  "D3D12 要求 D3D12_TEXTURE_DATA_PITCH_ALIGNMENT 对齐；该拷贝被跳过。",
                  (unsigned long long)sourceBytesPerRow);
        return;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    if (destination->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        barriers.push_back(DX12TransitionBarrier(destination->GetResource(),
                                                 destination->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_DEST));
        destination->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
    }
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = destination->GetResource();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = mip + slice * destination->GetMipLevels();

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = source;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = sourceOffset;
    src.PlacedFootprint.Footprint.Format = destination->GetDXGIFormat();

    uint32_t mipWidth = 0, mipHeight = 0;
    DX12Util::GetMipDimensions(destination->GetWidth(), destination->GetHeight(), mip, mipWidth, mipHeight);
    src.PlacedFootprint.Footprint.Width = (size.x > 0) ? (UINT)size.x : mipWidth;
    src.PlacedFootprint.Footprint.Height = (size.y > 0) ? (UINT)size.y : mipHeight;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = (UINT)sourceBytesPerRow;

    D3D12_BOX box = {};
    box.left = 0;
    box.top = 0;
    box.front = 0;
    box.right = src.PlacedFootprint.Footprint.Width;
    box.bottom = src.PlacedFootprint.Footprint.Height;
    box.back = 1;

    // 块压缩格式必须按块边界对齐拷贝框
    AlignBoxToBlockBoundary(destination->GetTextureFormat(), box);

    mCommandList->CopyTextureRegion(&dst, (UINT)offset.x, (UINT)offset.y, 0, &src, &box);
}

void DX12BlitEncoder::CopyBufferToTexture(RCBufferPtr source, uint64_t sourceOffset,
                                          uint64_t sourceBytesPerRow, uint64_t sourceBytesPerImage,
                                          RCTexturePtr destination, uint32_t destinationSlice,
                                          uint32_t destinationMipLevel,
                                          const mathutil::Vector2i& destinationOffset,
                                          const mathutil::Vector2i& destinationSize)
{
    (void)sourceBytesPerImage;
    auto src = std::dynamic_pointer_cast<DX12RCBuffer>(source);
    auto dst = std::dynamic_pointer_cast<DX12TextureBase>(destination);
    if (!src || !dst)
    {
        return;
    }
    DoBufferToTexture(src->GetResource(), sourceOffset, sourceBytesPerRow, dst.get(),
                      destinationSlice, destinationMipLevel, destinationOffset, destinationSize);
}

void DX12BlitEncoder::DoTextureToBuffer(DX12TextureBase* source, uint32_t slice, uint32_t mip,
                                        const mathutil::Vector2i& offset,
                                        const mathutil::Vector2i& size,
                                        ID3D12Resource* destination, uint64_t destinationOffset,
                                        uint64_t destinationBytesPerRow)
{
    if (source == nullptr || destination == nullptr || mCommandList == nullptr)
    {
        return;
    }

    if (!IsRowPitchAligned(destinationBytesPerRow))
    {
        LOG_ERROR("[DX12] CopyTextureToBuffer: destinationBytesPerRow=%llu 未按 256 字节对齐；"
                  "D3D12 要求 D3D12_TEXTURE_DATA_PITCH_ALIGNMENT 对齐；该拷贝被跳过。",
                  (unsigned long long)destinationBytesPerRow);
        return;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    if (source->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_SOURCE)
    {
        barriers.push_back(DX12TransitionBarrier(source->GetResource(), source->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_SOURCE));
        source->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = destination;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Offset = destinationOffset;
    dst.PlacedFootprint.Footprint.Format = source->GetDXGIFormat();

    uint32_t mipWidth = 0, mipHeight = 0;
    DX12Util::GetMipDimensions(source->GetWidth(), source->GetHeight(), mip, mipWidth, mipHeight);
    dst.PlacedFootprint.Footprint.Width = (size.x > 0) ? (UINT)size.x : mipWidth;
    dst.PlacedFootprint.Footprint.Height = (size.y > 0) ? (UINT)size.y : mipHeight;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = (UINT)destinationBytesPerRow;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = source->GetResource();
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = mip + slice * source->GetMipLevels();

    D3D12_BOX box = {};
    box.left = (UINT)offset.x;
    box.top = (UINT)offset.y;
    box.front = 0;
    box.right = box.left + dst.PlacedFootprint.Footprint.Width;
    box.bottom = box.top + dst.PlacedFootprint.Footprint.Height;
    box.back = 1;

    // 块压缩格式必须按块边界对齐拷贝框
    AlignBoxToBlockBoundary(source->GetTextureFormat(), box);

    mCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
}

void DX12BlitEncoder::CopyTextureToBuffer(RCTexturePtr source, uint32_t sourceSlice,
                                          uint32_t sourceMipLevel,
                                          const mathutil::Vector2i& sourceOffset,
                                          const mathutil::Vector2i& sourceSize,
                                          RCBufferPtr destination, uint64_t destinationOffset,
                                          uint64_t destinationBytesPerRow,
                                          uint64_t destinationBytesPerImage)
{
    (void)destinationBytesPerImage;
    auto src = std::dynamic_pointer_cast<DX12TextureBase>(source);
    auto dst = std::dynamic_pointer_cast<DX12RCBuffer>(destination);
    if (!src || !dst)
    {
        return;
    }
    DoTextureToBuffer(src.get(), sourceSlice, sourceMipLevel, sourceOffset, sourceSize,
                      dst->GetResource(), destinationOffset, destinationBytesPerRow);
}

// ============================================================================
// Texture 到 Texture
// ============================================================================

void DX12BlitEncoder::CopyTextureToTexture(RCTexturePtr source, uint32_t sourceSlice,
                                           uint32_t sourceMipLevel,
                                           const mathutil::Vector2i& sourceOffset,
                                           const mathutil::Vector2i& sourceSize,
                                           RCTexturePtr destination, uint32_t destinationSlice,
                                           uint32_t destinationMipLevel,
                                           const mathutil::Vector2i& destinationOffset,
                                           const mathutil::Vector2i& destinationSize)
{
    auto src = std::dynamic_pointer_cast<DX12TextureBase>(source);
    auto dst = std::dynamic_pointer_cast<DX12TextureBase>(destination);
    if (!src || !dst || !mCommandList)
    {
        return;
    }

    // D3D12 没有 format conversion blit：CopyTextureRegion 要求源目标格式兼容
    if (src->GetDXGIFormat() != dst->GetDXGIFormat())
    {
        LOG_ERROR("[DX12] CopyTextureToTexture: 源格式 %d 与目标格式 %d 不同。"
                  "D3D12 的 CopyTextureRegion 不支持格式转换，该拷贝被跳过。",
                  (int)src->GetDXGIFormat(), (int)dst->GetDXGIFormat());
        return;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    if (src->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_SOURCE)
    {
        barriers.push_back(DX12TransitionBarrier(src->GetResource(), src->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_SOURCE));
        src->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
    if (dst->GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        barriers.push_back(DX12TransitionBarrier(dst->GetResource(), dst->GetCurrentState(),
                                                 D3D12_RESOURCE_STATE_COPY_DEST));
        dst->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
    }
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = src->GetResource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = sourceMipLevel + sourceSlice * src->GetMipLevels();

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dst->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = destinationMipLevel + destinationSlice * dst->GetMipLevels();

    D3D12_BOX box = {};
    box.left = (UINT)sourceOffset.x;
    box.top = (UINT)sourceOffset.y;
    box.front = 0;

    uint32_t srcMipWidth = 0, srcMipHeight = 0;
    DX12Util::GetMipDimensions(src->GetWidth(), src->GetHeight(), sourceMipLevel, srcMipWidth, srcMipHeight);
    box.right = box.left + ((sourceSize.x > 0) ? (UINT)sourceSize.x : srcMipWidth);
    box.bottom = box.top + ((sourceSize.y > 0) ? (UINT)sourceSize.y : srcMipHeight);
    box.back = 1;

    mCommandList->CopyTextureRegion(&dstLocation, (UINT)destinationOffset.x,
                                    (UINT)destinationOffset.y, 0, &srcLocation, &box);
    (void)destinationSize;
}

// ============================================================================
// Mipmap 生成 / 暂存填充
// ============================================================================

void DX12BlitEncoder::GenerateMipmaps(RCTexturePtr texture, uint32_t slice)
{
    auto dx12Texture = std::dynamic_pointer_cast<DX12TextureBase>(texture);
    if (!dx12Texture || dx12Texture->GetMipLevels() <= 1)
    {
        return;
    }

    // D3D12 没有 vkCmdBlitImage 的等价物：mipmap 生成必须自己写降采样 compute pass。
    // 引擎的资产在离线烘焙时已经生成完整 mip 链，因此运行时不依赖这个入口。
    LOG_ERROR("[DX12] GenerateMipmaps 尚未实现（D3D12 无 blit 命令，需要降采样 compute shader）。"
              "纹理 '%s'（%u mips）的 mip 链保持不变。",
              "?", dx12Texture->GetMipLevels());
    (void)slice;
}

void DX12BlitEncoder::GenerateMipmapsForRange(RCTexturePtr texture, uint32_t slice,
                                              uint32_t baseMipLevel, uint32_t levelCount)
{
    (void)baseMipLevel;
    (void)levelCount;
    GenerateMipmaps(texture, slice);
}

NAMESPACE_RENDERCORE_END
