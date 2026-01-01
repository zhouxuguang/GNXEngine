//
//  RenderDelegate.mm
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import "RenderDelegate.h"
#include "Runtime/RenderCore/include/RenderDevice.h"


//#include "testShader.h"
//#include "TestImageDecode.h"
#include "WeakPtrProxy.h"
#include "testSkybox.h"
#include "TestPost/TestPost.hpp"
#include "TestTransform.hpp"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/SceneNode.h"
#include "Runtime/RenderSystem/include/ArcballManipulate.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "TestComputeShader.hpp"
#include "Runtime/BaseLib/include/DateTime.h"
#include "Runtime/GNXEngine/include/FrameGraph/FrameGraph.h"
#include "Runtime/GNXEngine/include/FrameGraph/TransientResources.h"


static RenderDeviceType convertToRenderDeviceType(RenderType renderType)
{
    switch (renderType)
    {
        case RenderTypeGLES:
            return RenderDeviceType::GLES;
            
        case RenderTypeMetal:
            return RenderDeviceType::METAL;
            
        case RenderTypeVulkan:
            return RenderDeviceType::VULKAN;
            
        default:
            break;
    }
    
    return RenderDeviceType::GLES;
}

@implementation GNXRenderDelegate
{
    RenderDevicePtr mRenderdevice;
    CGSize mViewSize;
    
    //test
    RCTexturePtr renderTexture;
    RCTexturePtr depthStencilTexture;
    
    SceneManager* sceneManager;
    
    RCTexturePtr computeTexture;
    ComputePipelinePtr computePipeline;
    
    uint64_t lastTime;
    
    TransientResources* mTransientResources;
}

- (void)initRenderWithHandle:(nonnull CALayer *)layer andType:(RenderType)renderType
{
    mRenderdevice = CreateRenderDevice(convertToRenderDeviceType(renderType), (__bridge void*)layer);
    
    mTransientResources = new TransientResources(mRenderdevice);
}

- (void)resizeRender:(NSUInteger)width andHeight:(NSUInteger)height
{
    if (mRenderdevice)
    {
        mRenderdevice->Resize((uint32_t)width, (uint32_t)height);
    }
    mViewSize = CGSizeMake(width, height);
    
    //test
    renderTexture = mRenderdevice->CreateTexture2D(kTexFormatRGBA16Float,
                                                   TextureUsage::TextureUsageRenderTarget, width, height, 1);
    computeTexture = mRenderdevice->CreateTexture2D(kTexFormatRGBA16Float,
                                                    TextureUsage::TextureUsageRenderTarget, width, height, 1);
    
    depthStencilTexture = mRenderdevice->CreateTexture2D(kTexFormatDepth32FloatStencil8,
                                                         TextureUsage::TextureUsageRenderTarget,
                                                         width, height, 1);
    
    sceneManager = SceneManager::GetInstance();
    SkyBox* skybox = initSky(mRenderdevice);
    sceneManager->GetSkyBox()->AttachSkyBoxObject(skybox);
    initPostResource(mRenderdevice);
    
    //初始化相机
    CameraPtr cameraPtr = sceneManager->createCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(width) / height, 0.1f, 100.f);
    
    //初始化灯光信息
    Light * pointLight = sceneManager->createLight("mainLight", Light::LightType::PointLight);
    pointLight->setColor(Vector3f(1.0, 1.0, 1.0));
    //pointLight->setPosition(Vector3f(5.0, 8.0, 0.0));
    pointLight->setPosition(Vector3f(-1.0, -1.0, -1.0));
    pointLight->setFalloffStart(5);
    pointLight->setFalloffEnd(300);
    pointLight->setStrength(Vector3f(8.0, 8.0, 8.0));
    
    NSUInteger bytes = [[NSThread currentThread] stackSize];
    
    Quaternionf rotate;
    rotate.FromAngleAxis(90, Vector3f(1.0, 0.0, 0.0));
    //sceneManager->getRootNode()->createRendererNode("hat", "DamagedHelmet/glTF/DamagedHelmet.gltf");
    
    //gltf/BrainStem/glTF
    sceneManager->getRootNode()->createRendererNode("hat", "gltf/BrainStem/glTF/BrainStem.gltf",
                                                    Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(3, 3, 3));
    //sceneManager->getRootNode()->createRendererNode("hat", "skin/Woman.gltf", Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(0.01, 0.01, 0.01));
    
    //sceneManager->getRootNode()->createRendererNode("Marry", "asset/Marry.obj", Vector3f(0, -2.0, 0));
    
    //sceneManager->getRootNode()->createRendererNode("Marry", "nanosuit/nanosuit.obj", Vector3f(0, -4.0, 0));
    
    //TestADD();
    //computeTexture = TestImageGray();
    computePipeline = initTestimageGray();
    
    lastTime = GetTickNanoSeconds();
}

- (void)drawFrame
{
    uint64_t thisTime = GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    printf("deltaTime = %f\n", deltaTime);
    lastTime = thisTime;
    sceneManager->Update(deltaTime);
    
    CommandBufferPtr commandBuffer = mRenderdevice->CreateCommandBuffer();
    
    struct PassData 
    {
        FrameGraphResource colorTarget;
        FrameGraphResource depthStencilTarget;
    };
    
    FrameGraph frameGraph;
    const PassData& basePassData = frameGraph.AddPass<PassData>("BasePass",
                       [=](FrameGraph::Builder &builder, PassData &data)
    {
        FrameGraphTexture::Desc colorDesc;
        colorDesc.extent.width = mViewSize.width;
        colorDesc.extent.height = mViewSize.height;
        colorDesc.format = kTexFormatRGBA16Float;
        data.colorTarget = builder.create<FrameGraphTexture>("ColorTarget0", colorDesc);
        data.colorTarget = builder.write(data.colorTarget);
        
        FrameGraphTexture::Desc depthStencilDesc;
        depthStencilDesc.extent.width = mViewSize.width;
        depthStencilDesc.extent.height = mViewSize.height;
        depthStencilDesc.format = kTexFormatDepth32FloatStencil8;
        data.depthStencilTarget = builder.create<FrameGraphTexture>("depthStencilTarget", depthStencilDesc);
        data.depthStencilTarget = builder.write(data.depthStencilTarget);
    },
                       [=](const PassData &data, FrameGraphPassResources &resources, void *) 
    {
        FrameGraphTexture &colorTexture = resources.Get<FrameGraphTexture>(data.colorTarget);
        FrameGraphTexture &depthStencilTexture = resources.Get<FrameGraphTexture>(data.depthStencilTarget);
        
        RenderPass renderPass;
        RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
        colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
        colorAttachmentPtr->texture = colorTexture.texture;
        renderPass.colorAttachments.push_back(colorAttachmentPtr);
        
        renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
        renderPass.depthAttachment->texture = depthStencilTexture.texture;
        renderPass.depthAttachment->clearDepth = 1.0;
        
        renderPass.stencilAttachment = std::make_shared<RenderPassStencilAttachment>();
        renderPass.stencilAttachment->texture = depthStencilTexture.texture;
        renderPass.stencilAttachment->clearStencil = 0x00;

        renderPass.renderRegion = Rect2D(0, 0, mViewSize.width, mViewSize.height);
        RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
        
        sceneManager->Render(renderEncoder);
        
        renderEncoder->EndEncode();
    });
    
    frameGraph.AddPass("PresentPass", [=](FrameGraph::Builder &builder, FrameGraph::NoData &data)
    {
        builder.read(basePassData.colorTarget);
        
        // present的pass必须设置这个标记，要不然不会执行
        builder.setSideEffect();
    },
                       [=](const FrameGraph::NoData &data, FrameGraphPassResources &resources, void *)
    {
        FrameGraphTexture &colorTexture = resources.Get<FrameGraphTexture>(basePassData.colorTarget);
        
        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        testPost(renderEncoder, colorTexture.texture);
        renderEncoder->EndEncode();
        commandBuffer->PresentFrameBuffer();
    });
    
    frameGraph.Compile();
    frameGraph.Execute(nullptr, mTransientResources);
    
    return;
    
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

    renderPass.renderRegion = Rect2D(0, 0, mViewSize.width, mViewSize.height);
    RenderEncoderPtr renderEncoder1 = commandBuffer->CreateRenderEncoder(renderPass);
    
    sceneManager->Render(renderEncoder1);
    
    renderEncoder1->EndEncode();
    
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    testImageGrayDraw(computeEncoder, computePipeline, renderTexture, computeTexture);
    computeEncoder->EndEncode();
    
    RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    testPost(renderEncoder, computeTexture);
    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

@end
