//
//  VirtualTextureFeedback.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureFeedback.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

NS_RENDERSYSTEM_BEGIN

VirtualTextureFeedback::VirtualTextureFeedback(const mathutil::Vector2i& viewSize, uint32_t feedbackScale, const VirtualTextureConfig& config)
    : mScale(feedbackScale)
{
    mWidth  = viewSize.x / mScale;
    mHeight = viewSize.y / mScale;
    
    mFeedbackTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatR32Uint,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);
    
    mDepthTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatDepth32,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);
}

FeedbackResult VirtualTextureFeedback::ReadbackAndDecode()
{
    FeedbackResult result;
    result.feedbackWidth  = mWidth;
    result.feedbackHeight = mHeight;

    // TODO:
    //   1. 从 mFeedbackTarget 执行 GPU → CPU readback（PBO or map）。
    //   2. 遍历每个像素，调用 DecodePixel()。
    //   3. 插入 result.requestedPages（自动去重）。

    return result;
}

PageRequest VirtualTextureFeedback::DecodePixel(FeedbackPixel pixel)
{
    // 编码格式：
    //   bit 0   : valid 标记
    //   bit 1-4  : mipLevel（4位，支持最多16级）
    //   bit 5-12 : pageX（8 位）
    //   bit 13-20: pageY（8 位）

    PageRequest req;
    req.mipLevel = pixel & 0x1Fu;
    req.pageX    = (pixel >> 5)  & 0xFFu;
    req.pageY    = (pixel >> 13) & 0xFFu;
    return req;
}

NS_RENDERSYSTEM_END
