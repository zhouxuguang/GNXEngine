//
//  MTLCommandBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLCommandBuffer.h"
#include "MTLRenderEncoder.h"
#include "MTLComputeEncoder.h"
#include "MTLRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

MTLCommandBuffer::MTLCommandBuffer(id<MTLCommandQueue> commandQueue, CAMetalLayer *metalLayer,
                                   id<MTLTexture> depthTexture, id<MTLTexture> stencilTexture, id<MTLTexture> depthStencilTexture)
{
    @autoreleasepool 
    {
        mMetalLayer = metalLayer;
        mCommandBuffer = [commandQueue commandBuffer];
        mDepthTexture = depthTexture;
        mStencilTexture = stencilTexture;
        mDepthStencilTexture = depthStencilTexture;
    }
}

MTLCommandBuffer::~MTLCommandBuffer()
{
}

//创建默认的encoder，也就是屏幕渲染的encoder
RenderEncoderPtr MTLCommandBuffer::createDefaultRenderEncoder() const
{
    @autoreleasepool {
        mCurrentDrawable =  [mMetalLayer nextDrawable];
        id<MTLTexture> texture = mCurrentDrawable.texture;

        MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
        
        passDescriptor.depthAttachment.texture = mDepthStencilTexture;
        passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        passDescriptor.depthAttachment.clearDepth = 1.0;
        
        passDescriptor.stencilAttachment.texture = mDepthStencilTexture;
        passDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
        passDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
        passDescriptor.stencilAttachment.clearStencil = 0;
        
        id <MTLRenderCommandEncoder> commandEncoder = [mCommandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
        
        MTLViewport viewport;
        viewport.width = mMetalLayer.drawableSize.width;
        viewport.height = mMetalLayer.drawableSize.height;
        viewport.originX = 0;
        viewport.originY = 0;
        viewport.znear = 0;
        viewport.zfar = 1;
        [commandEncoder setViewport:viewport];
        
        FrameBufferFormat frameBufferFormat;
        frameBufferFormat.colorFormats.push_back(texture.pixelFormat);
        frameBufferFormat.depthFormat = mDepthStencilTexture.pixelFormat;
        frameBufferFormat.stencilFormat = mDepthStencilTexture.pixelFormat;
        
        return std::make_shared<MTLRenderEncoder>(commandEncoder, frameBufferFormat);
    }
}

RenderEncoderPtr MTLCommandBuffer::createRenderEncoder(const RenderPass& renderPass) const
{
    @autoreleasepool {
        FrameBufferFormat frameBufferFormat;
        frameBufferFormat.depthFormat = MTLPixelFormatInvalid;
        frameBufferFormat.stencilFormat = MTLPixelFormatInvalid;
        
        MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        
        for (size_t i = 0; i < renderPass.colorAttachments.size(); i ++)
        {
            RenderPassColorAttachmentPtr iter = renderPass.colorAttachments[i];
            if (!iter)
            {
                continue;
            }
            
            MTLRenderTexturePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLRenderTexture>(iter->texture);
            
            if (mtlRenderTexture == nullptr)
            {
                continue;
            }
            
            passDescriptor.colorAttachments[i].texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.colorAttachments[i].loadAction = MTLLoadActionClear;
            passDescriptor.colorAttachments[i].storeAction = MTLStoreActionStore;   //这里使用了memoryless的话，就不能store
            passDescriptor.colorAttachments[i].clearColor = MTLClearColorMake(iter->clearColor.red,
                                                                              iter->clearColor.green,
                                                                              iter->clearColor.blue,
                                                                              iter->clearColor.alpha);
            
            frameBufferFormat.colorFormats.push_back(mtlRenderTexture->getMTLTexture().pixelFormat);
        }
        
        if (renderPass.depthAttachment)
        {
            MTLRenderTexturePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLRenderTexture>(renderPass.depthAttachment->texture);
            
            passDescriptor.depthAttachment.texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
            passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
            passDescriptor.depthAttachment.clearDepth = renderPass.depthAttachment->clearDepth;
            frameBufferFormat.depthFormat = mtlRenderTexture->getMTLTexture().pixelFormat;
        }
        
        if (renderPass.stencilAttachment)
        {
            MTLRenderTexturePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLRenderTexture>(renderPass.stencilAttachment->texture);
            
            passDescriptor.stencilAttachment.texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
            passDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
            passDescriptor.stencilAttachment.clearStencil = renderPass.stencilAttachment->clearStencil;
            frameBufferFormat.stencilFormat = mtlRenderTexture->getMTLTexture().pixelFormat;
        }
        
        id <MTLRenderCommandEncoder> commandEncoder = [mCommandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
        
        MTLViewport viewport;
        viewport.width = renderPass.renderRegion.width;
        viewport.height = renderPass.renderRegion.height;
        viewport.originX = renderPass.renderRegion.offsetX;
        viewport.originY = renderPass.renderRegion.offsetY;
        viewport.znear = 0;
        viewport.zfar = 1;
        [commandEncoder setViewport:viewport];
        
        return std::make_shared<MTLRenderEncoder>(commandEncoder, frameBufferFormat);
    }
}

ComputeEncoderPtr MTLCommandBuffer::createComputeEncoder() const
{
    return std::make_shared<MTLComputeEncoder>(mCommandBuffer);
}

//呈现到屏幕上，上屏
void MTLCommandBuffer::presentFrameBuffer()
{
    @autoreleasepool
    {
        //[mCommandBuffer presentDrawable:mCurrentDrawable];
        
        id<MTLDrawable> currentDrawable = mCurrentDrawable;
        
        [mCommandBuffer addScheduledHandler:^(id<MTLCommandBuffer> _Nonnull commandBuffer)
         {
            [currentDrawable present];
        }];
        
        [mCommandBuffer commit];
    }
}

void MTLCommandBuffer::waitUntilCompleted()
{
    @autoreleasepool 
    {
        [mCommandBuffer commit];
        [mCommandBuffer waitUntilCompleted];
    }
}

NAMESPACE_RENDERCORE_END
