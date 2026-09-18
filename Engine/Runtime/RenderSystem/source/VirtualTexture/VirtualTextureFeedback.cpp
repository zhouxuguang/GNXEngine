//
//  VirtualTextureFeedback.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureFeedback.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/BlitEncoder.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include <algorithm>

NS_RENDERSYSTEM_BEGIN

namespace
{
    // D3D12 texture-to-buffer 行 pitch 需 256 字节对齐。
    constexpr uint32_t kFeedbackRowPitchAlignment = 256;

    inline uint32_t AlignUp(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) / alignment * alignment;
    }
}

VirtualTextureFeedback::VirtualTextureFeedback(const mathutil::Vector2i& viewSize, uint32_t feedbackScale, const VirtualTextureConfig& /*config*/)
    : mScale(std::max(feedbackScale, 1u))
{
    Resize(viewSize);
}

void VirtualTextureFeedback::Resize(const mathutil::Vector2i& viewSize)
{
    const uint32_t width = std::max(1, viewSize.x) / mScale;
    const uint32_t height = std::max(1, viewSize.y) / mScale;
    const uint32_t resolvedWidth = std::max(width, 1u);
    const uint32_t resolvedHeight = std::max(height, 1u);

    if (resolvedWidth == mWidth && resolvedHeight == mHeight &&
        mFeedbackTarget && mDepthTarget && mStagingBuffer)
    {
        return;
    }

    // 避免 resize 释放仍在回读的资源。
    if (mPendingCommandBuffer)
    {
        mPendingCommandBuffer->WaitUntilCompleted();
        mPendingCommandBuffer.reset();
    }

    mWidth = resolvedWidth;
    mHeight = resolvedHeight;

    mFeedbackTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatR32Uint,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);

    mDepthTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatDepth16,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);

    // GPU→CPU readback buffer。
    mPitch = AlignUp(mWidth * sizeof(uint32_t), kFeedbackRowPitchAlignment);
    RCBufferDesc stagingDesc(mPitch * mHeight,
                             RCBufferUsage::TransferDst,
                             StorageModeShared);
    mStagingBuffer = RenderCore::GetRenderDevice()->CreateBuffer(stagingDesc);
    mHasRenderedFrame = false;
}

FeedbackResult VirtualTextureFeedback::ReadbackAndDecode()
{
    FeedbackResult result;
    result.feedbackWidth  = mWidth;
    result.feedbackHeight = mHeight;

    if (!mHasRenderedFrame || !mFeedbackTarget || !mStagingBuffer)
    {
        return result;
    }
    mHasRenderedFrame = false;

    // 等待上一帧主命令缓冲区中的回读。
    if (mPendingCommandBuffer)
    {
        mPendingCommandBuffer->WaitUntilCompleted();
        mPendingCommandBuffer.reset();
    }

    // Step 2: Map + Decode
    void* mapped = mStagingBuffer->Map();
    if (mapped) 
    {
        const uint8_t* base = static_cast<const uint8_t*>(mapped);
        for (uint32_t y = 0; y < mHeight; ++y)
        {
            const uint32_t* row = reinterpret_cast<const uint32_t*>(base + static_cast<size_t>(y) * mPitch);
            for (uint32_t x = 0; x < mWidth; ++x)
            {
                const uint32_t pixel = row[x];
                // Feedback shader 用 bit31 标记有效像素。不能只判断非零：
                // 初始化/resize 后的未定义显存可能包含其它非零值。
                if ((pixel & (1u << 31)) == 0u)
                {
                    continue;
                }
                result.requestedPages.insert(DecodePixel(pixel));
            }
        }
        mStagingBuffer->Unmap();
    }

    return result;
}

void VirtualTextureFeedback::RecordReadback(const CommandBufferPtr& commandBuffer)
{
    if (!commandBuffer || !mFeedbackTarget || !mStagingBuffer)
    {
        return;
    }

    BlitEncoderPtr blit = commandBuffer->CreateBlitEncoder();
    if (!blit)
    {
        return;
    }

    blit->CopyTextureToBuffer(mFeedbackTarget, 0, 0,
                              mathutil::Vector2i(0, 0), mathutil::Vector2i(mWidth, mHeight),
                              mStagingBuffer, 0, mPitch, 0);
    blit->EndEncode();

    mPendingCommandBuffer = commandBuffer;
    mHasRenderedFrame = true;
}

PageRequest VirtualTextureFeedback::DecodePixel(FeedbackPixel pixel)
{
    // 编码格式：
    //   bit 31   : valid 标记
    //   bit 0-4  : mipLevel（5位，支持最多32级）
    //   bit 5-12 : pageX（8 位）
    //   bit 13-20: pageY（8 位）

    PageRequest req;
    req.mipLevel = pixel & 0x1Fu;
    req.pageX    = (pixel >> 5)  & 0xFFu;
    req.pageY    = (pixel >> 13) & 0xFFu;
    return req;
}

NS_RENDERSYSTEM_END
