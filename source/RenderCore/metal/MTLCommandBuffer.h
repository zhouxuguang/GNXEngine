//
//  MTLCommandBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE
#define GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE

#include "MTLRenderDefine.h"
#include "CommandBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

// 命令缓冲区
class MTLCommandBuffer : public CommandBuffer
{
public:
    MTLCommandBuffer(id<MTLCommandQueue> commandQueue, CAMetalLayer *metalLayer,
                     id<MTLTexture> depthTexture, id<MTLTexture> stencilTexture, id<MTLTexture> depthStencilTexture);
    
    ~MTLCommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    virtual RenderEncoderPtr createDefaultRenderEncoder() const;
    
    virtual RenderEncoderPtr createRenderEncoder(const RenderPass& renderPass) const;
    
    // 创建计算着色器的encoder
    virtual ComputeEncoderPtr createComputeEncoder() const;
    
    //呈现到屏幕上，上屏
    virtual void presentFrameBuffer();
    
    //等待命令缓冲区执行完成
    virtual void waitUntilCompleted();
    
private:
    id<MTLCommandBuffer> mCommandBuffer;
    CAMetalLayer *mMetalLayer = nil;
    mutable id<CAMetalDrawable> mCurrentDrawable = nil;
    
    id<MTLTexture> mDepthTexture = nil;
    id<MTLTexture> mStencilTexture = nil;
    id<MTLTexture> mDepthStencilTexture = nil;
};

typedef std::shared_ptr<MTLCommandBuffer> MTLCommandBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMMANS_BUFFER_INCLUDE */
