//
//  RenderDelegate.mm
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import "RenderDelegate.h"
#include "RenderCore/RenderDevice.h"


#include "testShader.h"
#include "TestImageDecode.h"
#include "WeakPtrProxy.h"
#include "testSkybox.h"
#include "TestPost/TestPost.hpp"
#include "TestMesh/TestMesh.hpp"
#include "TestTransform.hpp"
#include "RenderSystem/SceneNode.h"

static rendercore::RenderDeviceType convertToRenderDeviceType(RenderType renderType)
{
    switch (renderType)
    {
        case RenderTypeGLES:
            return rendercore::RenderDeviceType::GLES;
            
        case RenderTypeMetal:
            return rendercore::RenderDeviceType::METAL;
            
        default:
            break;
    }
    
    return rendercore::RenderDeviceType::GLES;
}

@implementation GNXRenderDelegate
{
    rendercore::RenderDevicePtr mRenderdevice;
    CGSize mViewSize;
    
    //test
    rendercore::RenderTexturePtr renderTexture;
    rendercore::RenderTexturePtr depthStencilTexture;
}

- (void)initRenderWithHandle:(nonnull CALayer *)layer andType:(RenderType)renderType
{
    mRenderdevice = rendercore::createRenderDevice(convertToRenderDeviceType(renderType), (__bridge void*)layer);
}

- (void)resizeRender:(NSUInteger)width andHeight:(NSUInteger)height
{
    if (mRenderdevice)
    {
        mRenderdevice->resize((uint32_t)width, (uint32_t)height);
    }
    mViewSize = CGSizeMake(width, height);
    
    //test
    TextureDescriptor textureDescriptor;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.mipmaped = false;
    textureDescriptor.format = kTexFormatRGBA32;
    renderTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.mipmaped = false;
    textureDescriptor.format = kTexFormatDepth32FloatStencil8;
    depthStencilTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
    TestTransform();
    
    initSky(mRenderdevice);
    initPostResource(mRenderdevice);
    initMesh(mRenderdevice);
}

- (void)drawFrame
{
    CommandBufferPtr commandBuffer = mRenderdevice->createCommandBuffer();
    
    RenderPass renderPass;
    RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
    colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
    colorAttachmentPtr->texture = renderTexture;
    renderPass.colorAttachments.push_back(colorAttachmentPtr);
    
    renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
    renderPass.depthAttachment->texture = depthStencilTexture;
    renderPass.depthAttachment->clearDepth = 1.0;
    
    renderPass.stencilAttachment = std::make_shared<RenderPassStencilAttachment>();
    renderPass.stencilAttachment->texture = depthStencilTexture;
    renderPass.stencilAttachment->clearStencil = 0x00;

    renderPass.renderRegion = rendercore::Rect2D(0, 0, mViewSize.width, mViewSize.height);
    RenderEncoderPtr renderEncoder1 = commandBuffer->createRenderEncoder(renderPass);
    
    //testShader(mRenderdevice);
    
    RenderSystem::SceneNode *node = new RenderSystem::SceneNode;
    RenderSystem::TransformComponent* transform = new RenderSystem::TransformComponent;
    node->AddComponent(transform);
    
    RenderSystem::TransformComponent* com = node->QueryComponentT<RenderSystem::TransformComponent>();
    
    drawSky(renderEncoder1);
    
    testMesh(renderEncoder1);
    
    renderEncoder1->endEncoder();
    
    RenderEncoderPtr renderEncoder = commandBuffer->createDefaultRenderEncoder();
    testPost(renderEncoder, renderTexture);
    
    renderEncoder->endEncoder();
    commandBuffer->presentFrameBuffer();
}




@end
