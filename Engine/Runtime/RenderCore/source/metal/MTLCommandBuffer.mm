//
//  MTLCommandBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLCommandBuffer.h"
#include "MTLRenderEncoder.h"
#include "MTLComputeEncoder.h"
#include "MTLBlitEncoder.h"
#include "MTLTextureBase.h"
#include "MTLRCBuffer.h"

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
        mStencilTexture = depthStencilTexture;
        mDepthStencilTexture = depthStencilTexture;
        mMetalDevice = [commandQueue device];
    }
}

MTLCommandBuffer::~MTLCommandBuffer()
{
    @autoreleasepool
    {
        mFenceMap.clear();
    }
}

//创建默认的encoder，也就是屏幕渲染的encoder
RenderEncoderPtr MTLCommandBuffer::CreateDefaultRenderEncoder(const ClearColor& clearColor) const
{
    @autoreleasepool 
    {
        mCurrentDrawable =  [mMetalLayer nextDrawable];
        id<MTLTexture> texture = mCurrentDrawable.texture;

        MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clearColor.red, clearColor.green, clearColor.blue, clearColor.alpha);
        
        passDescriptor.depthAttachment.texture = mDepthStencilTexture;
        passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        passDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        passDescriptor.depthAttachment.clearDepth = DepthConfig::GetDefaultClearDepth();
        
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
        
        auto encoder = std::make_shared<MTLRenderEncoder>(commandEncoder, frameBufferFormat);
        encoder->SetFenceCallback([this](id<MTLRenderCommandEncoder> enc) 
        {
            this->OnEncoderEnding(enc);
        });
        return encoder;
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

        // 创建 encoder + 绑定 fence 回调
        auto encoder = std::make_shared<MTLRenderEncoder>(commandEncoder, frameBufferFormat);
        encoder->SetFenceCallback([this](id<MTLRenderCommandEncoder> enc) 
        {
            this->OnEncoderEnding(enc);
        });
        return encoder;
    }
}

ComputeEncoderPtr MTLCommandBuffer::CreateComputeEncoder() const
{
    return std::make_shared<MTLComputeEncoder>(mCommandBuffer);
}

BlitEncoderPtr MTLCommandBuffer::CreateBlitEncoder() const
{
    return std::make_shared<MTLBlitEncoder>(mCommandBuffer);
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
        [mCommandBuffer waitUntilCompleted];
        mCommandBuffer = nil;
    }
}

void MTLCommandBuffer::Submit()
{
    @autoreleasepool
    {
        [mCommandBuffer commit];
    }
}

void MTLCommandBuffer::BeginDebugGroup(const char* name, const float color[4])
{
    @autoreleasepool
    {
        NSString *strName = @"";
        if (name)
        {
            strName = [NSString stringWithUTF8String:name];
        }
        [mCommandBuffer pushDebugGroup:strName];
    }
}

void MTLCommandBuffer::EndDebugGroup()
{
    @autoreleasepool
    {
        [mCommandBuffer popDebugGroup];
    }
}

void MTLCommandBuffer::ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType)
{
    if (!texture)
        return;

    // 判断本次访问是读还是写
    bool isWrite = ((accessType & ResourceAccessType::ColorAttachment) != static_cast<ResourceAccessType>(0)) ||
                   ((accessType & ResourceAccessType::DepthStencilAttachment) != static_cast<ResourceAccessType>(0)) ||
                   ((accessType & ResourceAccessType::ComputeShaderWrite) != static_cast<ResourceAccessType>(0)) ||
                   ((accessType & ResourceAccessType::TransferDst) != static_cast<ResourceAccessType>(0));

    uint64_t resId = reinterpret_cast<uint64_t>(texture.get());
    FenceOp op;
    op.resourceId = resId;
    op.stage = AccessTypeToStage(accessType);
    op.type = isWrite ? FenceOp::Update : FenceOp::Wait;

    mPendingFenceOps.push_back(op);
    mFenceOpsDirty = true;
}

void MTLCommandBuffer::ResourceBarrier(RCBufferPtr buffer, ResourceAccessType accessType)
{
    if (!buffer)
        return;

    bool isWrite = ((accessType & ResourceAccessType::ComputeShaderWrite) != static_cast<ResourceAccessType>(0)) ||
                   ((accessType & ResourceAccessType::TransferDst) != static_cast<ResourceAccessType>(0));

    uint64_t resId = reinterpret_cast<uint64_t>(buffer.get());
    FenceOp op;
    op.resourceId = resId;
    op.stage = AccessTypeToStage(accessType);
    op.type = isWrite ? FenceOp::Update : FenceOp::Wait;

    mPendingFenceOps.push_back(op);
    mFenceOpsDirty = true;
}

// ...........................................................................
// Metal Fence 管理
// ...........................................................................

id<MTLFence> MTLCommandBuffer::GetOrCreateFence(uint64_t resourceId) const
{
    auto it = mFenceMap.find(resourceId);
    if (it != mFenceMap.end())
    {
        return it->second;
    }
    id<MTLFence> fence = [mMetalDevice newFence];
    mFenceMap[resourceId] = fence;
    return fence;
}

MTLRenderStages MTLCommandBuffer::AccessTypeToStage(ResourceAccessType access)
{
    if ((access & ResourceAccessType::ColorAttachment) != static_cast<ResourceAccessType>(0))
        return MTLRenderStageFragment;
    if ((access & ResourceAccessType::DepthStencilAttachment) != static_cast<ResourceAccessType>(0))
        return MTLRenderStageFragment;
    if ((access & ResourceAccessType::ShaderRead) != static_cast<ResourceAccessType>(0))
        return MTLRenderStageFragment;
    if ((access & ResourceAccessType::ComputeShaderRead) != static_cast<ResourceAccessType>(0) ||
        (access & ResourceAccessType::ComputeShaderWrite) != static_cast<ResourceAccessType>(0))
        return MTLRenderStageVertex | MTLRenderStageFragment;  // compute 是 vertex+fragment 之前的 stage
    if ((access & ResourceAccessType::TransferSrc) != static_cast<ResourceAccessType>(0) ||
        (access & ResourceAccessType::TransferDst) != static_cast<ResourceAccessType>(0))
        return MTLRenderStageVertex | MTLRenderStageFragment;
    return MTLRenderStageFragment;
}

void MTLCommandBuffer::OnEncoderEnding(id<MTLRenderCommandEncoder> encoder) const
{
    if (!mFenceOpsDirty || mPendingFenceOps.empty())
        return;

    // 按 resourceId 去重合并
    std::unordered_map<uint64_t, FenceOp> mergedOps;
    for (const auto& op : mPendingFenceOps)
    {
        mergedOps[op.resourceId] = op;
    }

    // 处理 Update 操作（合并 → 只需一次 updateFence，stage 取并集）
    id<MTLFence> updateFence = nil;
    MTLRenderStages updateStages = 0;
    for (const auto& pair : mergedOps)
    {
        if (pair.second.type == FenceOp::Update)
        {
            updateFence = GetOrCreateFence(pair.first);
            updateStages |= pair.second.stage;
        }
    }
    if (updateFence && updateStages != 0)
    {
        [encoder updateFence:updateFence afterStages:updateStages];
    }

    // 处理 Wait 操作
    for (const auto& pair : mergedOps)
    {
        if (pair.second.type == FenceOp::Wait)
        {
            id<MTLFence> waitFence = GetOrCreateFence(pair.first);
            [encoder waitForFence:waitFence beforeStages:pair.second.stage];
        }
    }

    mPendingFenceOps.clear();
    mFenceOpsDirty = false;
}

NAMESPACE_RENDERCORE_END
