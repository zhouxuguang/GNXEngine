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
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "TestComputeShader.hpp"
#include "Runtime/BaseLib/include/DateTime.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraph.h"
#include "Runtime/RenderSystem/include/FrameGraph/TransientResources.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraphBlackboard.h"


static RenderDeviceType convertToRenderDeviceType(RenderType renderType)
{
    switch (renderType)
    {
        case RenderTypeMetal:
            return RenderDeviceType::METAL;
            
        case RenderTypeVulkan:
            return RenderDeviceType::VULKAN;
            
        default:
            break;
    }
    
    return RenderDeviceType::VULKAN;
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
    
    RenderSystem::TransientResources* mTransientResources;
}

- (void)initRenderWithHandle:(nonnull CALayer *)layer andType:(RenderType)renderType
{
    mRenderdevice = CreateRenderDevice(convertToRenderDeviceType(renderType), (__bridge void*)layer);
    
    mTransientResources = new RenderSystem::TransientResources(mRenderdevice);
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
    CameraPtr cameraPtr = sceneManager->CreateCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, width, height, 0.1f, 100.f);

    //初始化灯光信息
    Light * pointLight = sceneManager->CreateLight("mainLight", Light::LightType::PointLight);
    pointLight->setColor(Vector3f(1.0, 1.0, 1.0));
    //pointLight->setPosition(Vector3f(5.0, 8.0, 0.0));
    pointLight->setPosition(Vector3f(-1.0, -1.0, -1.0));
    pointLight->setFalloffStart(5);
    pointLight->setFalloffEnd(300);
    pointLight->setStrength(Vector3f(8.0, 8.0, 8.0));
    
    NSUInteger bytes = [[NSThread currentThread] stackSize];
    
    Quaternionf rotate;
    rotate.FromAngleAxis(90, Vector3f(1.0, 0.0, 0.0));
    //sceneManager->GetRootNode()->CreateRendererNode("hat", "DamagedHelmet/glTF/DamagedHelmet.gltf");

    //gltf/BrainStem/glTF
    sceneManager->GetRootNode()->CreateRendererNode("hat", "gltf/BrainStem/glTF/BrainStem.gltf",
                                                    Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(3, 3, 3));
    //sceneManager->GetRootNode()->CreateRendererNode("hat", "skin/Woman.gltf", Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(0.01, 0.01, 0.01));

    //sceneManager->GetRootNode()->CreateRendererNode("Marry", "asset/Marry.obj", Vector3f(0, -2.0, 0));

    //sceneManager->GetRootNode()->CreateRendererNode("Marry", "nanosuit/nanosuit.obj", Vector3f(0, -4.0, 0));
    
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

    // 使用单个命令缓冲区，这样可以利用 MTLDispatchTypeConcurrent 实现计算和图形的并发执行
    // 从Graphics队列创建命令缓冲区
    CommandQueuePtr graphicsQueue = mRenderdevice->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();

    if (!commandBuffer)
    {
        return;
    }
    
    struct PassData 
    {
        RenderSystem::FrameGraphResource colorTarget;
        RenderSystem::FrameGraphResource depthStencilTarget;
    };
    
    RenderSystem::FrameGraph frameGraph;
    const PassData& basePassData = frameGraph.AddPass<PassData>("BasePass",
    [=](RenderSystem::FrameGraph::Builder &builder, PassData &data)
    {
        RenderSystem::FrameGraphTexture::Desc colorDesc;
        colorDesc.extent.width = mViewSize.width;
        colorDesc.extent.height = mViewSize.height;
        colorDesc.format = kTexFormatRGBA16Float;
        data.colorTarget = builder.Create<RenderSystem::FrameGraphTexture>("ColorTarget0", colorDesc);
        data.colorTarget = builder.Write(data.colorTarget);
        
        RenderSystem::FrameGraphTexture::Desc depthStencilDesc;
        depthStencilDesc.extent.width = mViewSize.width;
        depthStencilDesc.extent.height = mViewSize.height;
        depthStencilDesc.format = kTexFormatDepth32FloatStencil8;
        data.depthStencilTarget = builder.Create<RenderSystem::FrameGraphTexture>("depthStencilTarget", depthStencilDesc);
        data.depthStencilTarget = builder.Write(data.depthStencilTarget);
    },
    [=](const PassData &data, RenderSystem::FrameGraphPassResources &resources, void *)
    {
        RenderSystem::FrameGraphTexture &colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.colorTarget);
        RenderSystem::FrameGraphTexture &depthStencilTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.depthStencilTarget);
        
        RenderPass renderPass;
        RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
        colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
        colorAttachmentPtr->texture = colorTexture.texture;
        renderPass.colorAttachments.push_back(colorAttachmentPtr);
        
        renderPass.depthAttachment = std::make_shared<RenderPassDepthAttachment>();
        renderPass.depthAttachment->texture = depthStencilTexture.texture;
        renderPass.depthAttachment->clearDepth = DepthConfig::GetDefaultClearDepth();
        
        renderPass.stencilAttachment = std::make_shared<RenderPassStencilAttachment>();
        renderPass.stencilAttachment->texture = depthStencilTexture.texture;
        renderPass.stencilAttachment->clearStencil = 0x00;
        
        float color[4] = {1.0, 0.0, 0.0, 1.0};
        SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

        renderPass.renderRegion = Rect2D(0, 0, mViewSize.width, mViewSize.height);
        RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
        
        sceneManager->Render(renderEncoder);
        
        renderEncoder->EndEncode();
    });
    
    // 图像灰度化的计算管线
    struct ComputePassData
    {
        RenderSystem::FrameGraphResource inputColor;
        RenderSystem::FrameGraphResource outputColor;
    };
    
    RenderSystem::FrameGraphBlackboard fgBlackboard;
    fgBlackboard.Add<ComputePassData>() = frameGraph.AddPass<ComputePassData>("GrayCompute",
    [=](RenderSystem::FrameGraph::Builder &builder, ComputePassData &data)
    {
        builder.EnableAsyncCompute(true);
        RenderSystem::FrameGraphTexture::Desc colorDesc;
        colorDesc.extent.width = mViewSize.width;
        colorDesc.extent.height = mViewSize.height;
        colorDesc.format = kTexFormatRGBA16Float;
        data.outputColor = builder.Create<RenderSystem::FrameGraphTexture>("grayColor", colorDesc);
        data.outputColor = builder.Write(data.outputColor);

        data.inputColor = builder.Read(basePassData.colorTarget);
    },
    [=](const ComputePassData &data, RenderSystem::FrameGraphPassResources &resources, void *)
    {
        RenderSystem::FrameGraphTexture &colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.inputColor);
        RenderSystem::FrameGraphTexture &grayTexture = resources.Get<RenderSystem::FrameGraphTexture>(data.outputColor);
        
        float color[4] = {1.0, 0.0, 0.0, 1.0};
        SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

        // 使用并发计算编码器（MTLDispatchTypeConcurrent）
        // 这样计算命令和图形命令可以在 GPU 上并发执行
        ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
        testImageGrayDraw(computeEncoder, computePipeline, colorTexture.texture, grayTexture.texture);
        computeEncoder->EndEncode();
    });
    
    const ComputePassData& computePassData = fgBlackboard.Get<ComputePassData>();
    frameGraph.AddPass("PresentPass",
    [=](RenderSystem::FrameGraph::Builder &builder, RenderSystem::FrameGraph::NoData &data)
    {
        builder.Read(computePassData.outputColor);

        // present的pass必须设置这个标记，要不然不会执行
        builder.SetSideEffect();
    },
    [=](const RenderSystem::FrameGraph::NoData &data, RenderSystem::FrameGraphPassResources &resources, void *)
    {
        RenderSystem::FrameGraphTexture &colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(computePassData.outputColor);
        
        float color[4] = {1.0, 0.0, 0.0, 1.0};
        SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        testPost(renderEncoder, colorTexture.texture);
        renderEncoder->EndEncode();
        commandBuffer->PresentFrameBuffer();
    });
    
    std::ofstream{"/Users/zhouxuguang/work/opensource/fg.txt"} << frameGraph;
    
    frameGraph.Compile();
    frameGraph.Execute(nullptr, mTransientResources);
    
    mTransientResources->Update(deltaTime);
}

@end
