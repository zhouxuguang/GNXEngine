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
    virtual RenderEncoderPtr CreateDefaultRenderEncoder() const;
    
    virtual RenderEncoderPtr CreateRenderEncoder(const RenderPass& renderPass) const;
    
    // 创建计算着色器的encoder
    virtual ComputeEncoderPtr CreateComputeEncoder() const;
    
    // 创建Blit encoder，用于资源拷贝操作
    virtual BlitEncoderPtr CreateBlitEncoder() const;
    
    //呈现到屏幕上，上屏
    virtual void PresentFrameBuffer();
    
    //等待命令缓冲区执行完成
    virtual void WaitUntilCompleted();
    
    //提交命令缓冲区（用于计算命令缓冲区）
    virtual void Submit();
    
    // 开始调试标记
    virtual void BeginDebugGroup(const char* name, const float color[4]);
    
    // 结束调试标记
    virtual void EndDebugGroup();
    
    /**
     * 通知命令缓冲区纹理资源即将被访问
     * 自动处理Vulkan的layout转换
     * @param texture 纹理资源
     * @param accessType 访问类型（读/写、着色器读取、颜色附件等）
     */
    virtual void ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType) override;
    
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
