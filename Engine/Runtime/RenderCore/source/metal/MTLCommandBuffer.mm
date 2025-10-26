//
//  MTLCommandBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLCommandBuffer.h"
#include "MTLRenderEncoder.h"
#include "MTLComputeEncoder.h"
#include "MTLTextureBase.h"

NAMESPACE_RENDERCORE_BEGIN

static MTLLoadAction GetLoadAction(AttachmentLoadOp loadOp)
{
    MTLLoadAction resultOP = MTLLoadActionLoad;
    switch (loadOp)
    {
        case ATTACHMENT_LOAD_OP_LOAD:
            resultOP = MTLLoadActionLoad;
            break;
            
        case ATTACHMENT_LOAD_OP_CLEAR:
            resultOP = MTLLoadActionClear;
            break;
            
        case ATTACHMENT_LOAD_OP_DONT_CARE:
            resultOP = MTLLoadActionDontCare;
            break;
            
        default:
            break;
    }
    
    return resultOP;
}

static MTLStoreAction GetStoreAction(AttachmentStoreOp storeOp)
{
    MTLStoreAction resultOP = MTLStoreActionDontCare;
    switch (storeOp)
    {
        case ATTACHMENT_STORE_OP_STORE:
            resultOP = MTLStoreActionStore;
            break;
            
        case ATTACHMENT_STORE_OP_DONT_CARE:
            resultOP = MTLStoreActionDontCare;
            break;
            
        default:
            break;
    }
    
    return resultOP;
}

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
RenderEncoderPtr MTLCommandBuffer::CreateDefaultRenderEncoder() const
{
    @autoreleasepool 
    {
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

RenderEncoderPtr MTLCommandBuffer::CreateRenderEncoder(const RenderPass& renderPass) const
{
    @autoreleasepool 
    {
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
            
            MTLTextureBasePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLTextureBase>(iter->texture);
            
            if (mtlRenderTexture == nullptr)
            {
                continue;
            }
            
            passDescriptor.colorAttachments[i].texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.colorAttachments[i].level = iter->level;
            
            if (renderPass.layerCount > 1)
            {
                if (mtlRenderTexture->GetTextureType() == TextureType_3D)
                {
                    passDescriptor.colorAttachments[i].depthPlane = iter->slice;   //3d texture
                }
                else
                {
                    passDescriptor.colorAttachments[i].slice = iter->slice;
                }
            }
            
            passDescriptor.colorAttachments[i].loadAction = GetLoadAction(iter->loadOp);
            passDescriptor.colorAttachments[i].storeAction = GetStoreAction(iter->storeOp);   //这里使用了memoryless的话，就不能store
            passDescriptor.colorAttachments[i].clearColor = MTLClearColorMake(iter->clearColor.red,
                                                                              iter->clearColor.green,
                                                                              iter->clearColor.blue,
                                                                              iter->clearColor.alpha);
            
            frameBufferFormat.colorFormats.push_back(mtlRenderTexture->getMTLTexture().pixelFormat);
        }
        
        if (renderPass.depthAttachment)
        {
            MTLTextureBasePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLTextureBase>(renderPass.depthAttachment->texture);
            
            if (renderPass.layerCount > 1)
            {
                if (mtlRenderTexture->GetTextureType() == TextureType_3D)
                {
                    passDescriptor.depthAttachment.depthPlane = renderPass.depthAttachment->slice;   //3d texture
                }
                else
                {
                    passDescriptor.depthAttachment.slice = renderPass.depthAttachment->slice;
                }
            }
            
            passDescriptor.depthAttachment.texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.depthAttachment.level = renderPass.depthAttachment->level;
            
            passDescriptor.depthAttachment.loadAction = GetLoadAction(renderPass.depthAttachment->loadOp);
            passDescriptor.depthAttachment.storeAction = GetStoreAction(renderPass.depthAttachment->storeOp);
            passDescriptor.depthAttachment.clearDepth = renderPass.depthAttachment->clearDepth;
            frameBufferFormat.depthFormat = mtlRenderTexture->getMTLTexture().pixelFormat;
        }
        
        if (renderPass.stencilAttachment)
        {
            MTLTextureBasePtr mtlRenderTexture = std::dynamic_pointer_cast<MTLTextureBase>(renderPass.stencilAttachment->texture);
            
            if (renderPass.layerCount > 1)
            {
                if (mtlRenderTexture->GetTextureType() == TextureType_3D)
                {
                    passDescriptor.stencilAttachment.depthPlane = renderPass.stencilAttachment->slice;   //3d texture
                }
                else
                {
                    passDescriptor.stencilAttachment.slice = renderPass.stencilAttachment->slice;
                }
            }
            
            passDescriptor.stencilAttachment.texture = mtlRenderTexture->getMTLTexture();
            passDescriptor.stencilAttachment.level = renderPass.stencilAttachment->level;
            
            passDescriptor.stencilAttachment.loadAction = GetLoadAction(renderPass.stencilAttachment->loadOp);
            passDescriptor.stencilAttachment.storeAction = GetStoreAction(renderPass.stencilAttachment->storeOp);
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

ComputeEncoderPtr MTLCommandBuffer::CreateComputeEncoder() const
{
    return std::make_shared<MTLComputeEncoder>(mCommandBuffer);
}

//呈现到屏幕上，上屏
void MTLCommandBuffer::PresentFrameBuffer()
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

void MTLCommandBuffer::WaitUntilCompleted()
{
    @autoreleasepool 
    {
        [mCommandBuffer commit];
        [mCommandBuffer waitUntilCompleted];
    }
}

NAMESPACE_RENDERCORE_END
