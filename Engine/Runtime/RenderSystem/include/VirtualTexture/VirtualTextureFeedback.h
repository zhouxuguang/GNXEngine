//
//  VirtualTextureFeedback.h
//  GNXEngine
//
//  Feedback pass 管理：负责降分辨率渲染场景以确定
//  当前帧需要哪些虚拟 tile，将结果 readback 到 CPU，
//  并解析出需要加载/保持的 page 列表。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FEEDBACK_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FEEDBACK_H

#include "VirtualTextureDefines.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include <vector>
#include <unordered_set>

NS_RENDERSYSTEM_BEGIN

/// Feedback pass 结果：当前帧所需的虚拟 tile 集合。
struct FeedbackResult
{
    std::unordered_set<PageRequest, std::hash<PageRequest>> requestedPages;
    uint32_t feedbackWidth;
    uint32_t feedbackHeight;
};

/// Feedback pass 管理器。
class RENDERSYSTEM_API VirtualTextureFeedback
{
public:
    VirtualTextureFeedback(const mathutil::Vector2i& viewSize, uint32_t feedbackScale, const VirtualTextureConfig& config);
    ~VirtualTextureFeedback() = default;

    VirtualTextureFeedback(const VirtualTextureFeedback&) = delete;
    VirtualTextureFeedback& operator=(const VirtualTextureFeedback&) = delete;

    const RCTexturePtr GetFeedbackTarget() const { return mFeedbackTarget; }
    const RCTexturePtr GetDepthTarget() const { return mDepthTarget; }

    /// 执行 readback 并解析 feedback buffer 为 page 请求集合。
    FeedbackResult ReadbackAndDecode();

    uint32_t GetWidth()  const { return mWidth; }
    uint32_t GetHeight() const { return mHeight; }

private:
    RCTexture2DPtr mFeedbackTarget = nullptr;
    RCTexture2DPtr mDepthTarget = nullptr;
    uint32_t mWidth  = 0;
    uint32_t mHeight = 0;
    uint32_t mScale  = 16;

    /// 解码单个 FeedbackPixel → PageRequest。
    static PageRequest DecodePixel(FeedbackPixel pixel);
};

using VirtualTextureFeedbackPtr = std::shared_ptr<VirtualTextureFeedback>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FEEDBACK_H */
