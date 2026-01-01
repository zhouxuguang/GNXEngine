//
//  RenderDelegate.mm
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import "RenderDelegate.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

#include <fstream>

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
    
    GNXEngine::TransientResources* mTransientResources;
}

- (void)initRenderWithHandle:(nonnull CALayer *)layer andType:(RenderType)renderType
{
    mRenderdevice = CreateRenderDevice(convertToRenderDeviceType(renderType), (__bridge void*)layer);
    
    mTransientResources = new GNXEngine::TransientResources(mRenderdevice);
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
        GNXEngine::FrameGraphResource colorTarget;
        GNXEngine::FrameGraphResource depthStencilTarget;
    };
    
    GNXEngine::FrameGraph frameGraph;
    const PassData& basePassData = frameGraph.AddPass<PassData>("BasePass",
    [=](GNXEngine::FrameGraph::Builder &builder, PassData &data)
    {
        GNXEngine::FrameGraphTexture::Desc colorDesc;
        colorDesc.extent.width = mViewSize.width;
        colorDesc.extent.height = mViewSize.height;
        colorDesc.format = kTexFormatRGBA16Float;
        data.colorTarget = builder.create<GNXEngine::FrameGraphTexture>("ColorTarget0", colorDesc);
        data.colorTarget = builder.write(data.colorTarget);
        
        GNXEngine::FrameGraphTexture::Desc depthStencilDesc;
        depthStencilDesc.extent.width = mViewSize.width;
        depthStencilDesc.extent.height = mViewSize.height;
        depthStencilDesc.format = kTexFormatDepth32FloatStencil8;
        data.depthStencilTarget = builder.create<GNXEngine::FrameGraphTexture>("depthStencilTarget", depthStencilDesc);
        data.depthStencilTarget = builder.write(data.depthStencilTarget);
    },
    [=](const PassData &data, GNXEngine::FrameGraphPassResources &resources, void *)
    {
        GNXEngine::FrameGraphTexture &colorTexture = resources.Get<GNXEngine::FrameGraphTexture>(data.colorTarget);
        GNXEngine::FrameGraphTexture &depthStencilTexture = resources.Get<GNXEngine::FrameGraphTexture>(data.depthStencilTarget);
        
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
    
    // 图像灰度化的计算管线
    struct ComputePassData
    {
        GNXEngine::FrameGraphResource inputColor;
        GNXEngine::FrameGraphResource outputColor;
    };
    const ComputePassData &computePassData = frameGraph.AddPass<ComputePassData>("GrayCompute",
    [=](GNXEngine::FrameGraph::Builder &builder, ComputePassData &data)
    {
        GNXEngine::FrameGraphTexture::Desc colorDesc;
        colorDesc.extent.width = mViewSize.width;
        colorDesc.extent.height = mViewSize.height;
        colorDesc.format = kTexFormatRGBA16Float;
        data.outputColor = builder.create<GNXEngine::FrameGraphTexture>("grayColor", colorDesc);
        data.outputColor = builder.write(data.outputColor);

        data.inputColor = builder.read(basePassData.colorTarget);
    },
    [=](const ComputePassData &data, GNXEngine::FrameGraphPassResources &resources, void *)
    {
        GNXEngine::FrameGraphTexture &colorTexture = resources.Get<GNXEngine::FrameGraphTexture>(data.inputColor);
        GNXEngine::FrameGraphTexture &grayTexture = resources.Get<GNXEngine::FrameGraphTexture>(data.outputColor);
        
        ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
        testImageGrayDraw(computeEncoder, computePipeline, colorTexture.texture, grayTexture.texture);
        computeEncoder->EndEncode();
    });
    
    frameGraph.AddPass("PresentPass", 
    [=](GNXEngine::FrameGraph::Builder &builder, GNXEngine::FrameGraph::NoData &data)
    {
        builder.read(computePassData.outputColor);
        
        // present的pass必须设置这个标记，要不然不会执行
        builder.setSideEffect();
    },
    [=](const GNXEngine::FrameGraph::NoData &data, GNXEngine::FrameGraphPassResources &resources, void *)
    {
        GNXEngine::FrameGraphTexture &colorTexture = resources.Get<GNXEngine::FrameGraphTexture>(computePassData.outputColor);
        
        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        testPost(renderEncoder, colorTexture.texture);
        renderEncoder->EndEncode();
        commandBuffer->PresentFrameBuffer();
    });
    
    std::ofstream{"/Users/zhouxuguang/work/opensource/fg.txt"} << frameGraph;
    
    frameGraph.Compile();
    frameGraph.Execute(nullptr, mTransientResources);
    
    mTransientResources->Update(deltaTime);
    
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
