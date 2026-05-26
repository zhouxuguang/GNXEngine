//
//  VirtualTextureFeedback.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureFeedback.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/CommandQueue.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/BlitEncoder.h"
#include "Runtime/MathUtil/include/Vector2.h"

NS_RENDERSYSTEM_BEGIN

VirtualTextureFeedback::VirtualTextureFeedback(const mathutil::Vector2i& viewSize, uint32_t feedbackScale, const VirtualTextureConfig& config)
    : mScale(feedbackScale)
{
    mWidth  = viewSize.x / mScale;
    mHeight = viewSize.y / mScale;
    
    mFeedbackTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatR32Uint,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);

    mDepthTarget = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatDepth16,
                            RenderCore::TextureUsage::TextureUsageRenderTarget, mWidth, mHeight, 1);

    // 创建 GPU→CPU readback staging buffer（StorageModeShared 才能.Map()）
    RCBufferDesc stagingDesc(mWidth * mHeight * sizeof(uint32_t),
                             RCBufferUsage::TransferDst,
                             StorageModeShared);
    mStagingBuffer = RenderCore::GetRenderDevice()->CreateBuffer(stagingDesc);
}

FeedbackResult VirtualTextureFeedback::ReadbackAndDecode()
{
    FeedbackResult result;
    result.feedbackWidth  = mWidth;
    result.feedbackHeight = mHeight;

    if (!mStagingBuffer) 
    {
        return result;
    }

    RenderCore::RenderDevicePtr device = RenderCore::GetRenderDevice();

    // Step 1: GPU Copy: Texture → Staging Buffer
    CommandQueuePtr queue = device->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr cmdBuf = queue->CreateCommandBuffer();
    
    // 从 ColorAttachment → TransferSrc（准备拷贝）
    cmdBuf->ResourceBarrier(mFeedbackTarget, RenderCore::ResourceAccessType::TransferSrc);
    
    BlitEncoderPtr blit = cmdBuf->CreateBlitEncoder();

    blit->CopyTextureToBuffer(mFeedbackTarget, 0, 0, mathutil::Vector2i(0, 0), mathutil::Vector2i(mWidth, mHeight),
                               mStagingBuffer, 0, mWidth * sizeof(uint32_t), 0);
    blit->EndEncode();

    // 从 TransferSrc → ColorAttachment（为下一帧的 Feedback Pass 做准备）
    cmdBuf->ResourceBarrier(mFeedbackTarget, RenderCore::ResourceAccessType::ColorAttachment);

    cmdBuf->Submit();
    cmdBuf->WaitUntilCompleted();

    // Step 2: Map + Decode
    void* mapped = mStagingBuffer->Map();
    if (mapped) 
    {
        const uint32_t* pixels = static_cast<const uint32_t*>(mapped);
        uint32_t total = mWidth * mHeight;
        for (uint32_t i = 0; i < total; ++i) 
        {
            if (pixels[i] == 0)
            {
                continue;
            }
            result.requestedPages.insert(DecodePixel(pixels[i]));
        }
        mStagingBuffer->Unmap();
    }

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
