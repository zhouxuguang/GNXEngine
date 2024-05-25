//
//  RenderDelegate.mm
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import "RenderDelegate.h"
#include "RenderCore/RenderDevice.h"


//#include "testShader.h"
//#include "TestImageDecode.h"
#include "WeakPtrProxy.h"
#include "testSkybox.h"
#include "TestPost/TestPost.hpp"
#include "TestTransform.hpp"
#include "RenderSystem/SceneManager.h"
#include "RenderSystem/SceneNode.h"
#include "RenderSystem/ArcballManipulate.h"
#include "MathUtil/Vector3.h"
#include "RenderSystem/SkyBoxNode.h"
#include "ImageCodec/ImageDecoder.h"
#include "RenderSystem/RenderEngine.h"
#include "TestComputeShader.hpp"
#include "BaseLib/DateTime.h"


static RenderDeviceType convertToRenderDeviceType(RenderType renderType)
{
    switch (renderType)
    {
        case RenderTypeGLES:
            return RenderDeviceType::GLES;
            
        case RenderTypeMetal:
            return RenderDeviceType::METAL;
            
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
    RenderTexturePtr renderTexture;
    RenderTexturePtr depthStencilTexture;
    
    SceneManager* sceneManager;
    
    RenderTexturePtr computeTexture;
    ComputePipelinePtr computePipeline;
    
    uint64_t lastTime;
}

- (void)initRenderWithHandle:(nonnull CALayer *)layer andType:(RenderType)renderType
{
    mRenderdevice = createRenderDevice(convertToRenderDeviceType(renderType), (__bridge void*)layer);
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
    textureDescriptor.format = kTexFormatRGBA16Float;
    renderTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    computeTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.mipmaped = false;
    textureDescriptor.format = kTexFormatDepth32FloatStencil8;
    depthStencilTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
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

    renderPass.renderRegion = Rect2D(0, 0, mViewSize.width, mViewSize.height);
    RenderEncoderPtr renderEncoder1 = commandBuffer->createRenderEncoder(renderPass);
    
    sceneManager->Render(renderEncoder1);
    
    renderEncoder1->EndEncode();
    
    ComputeEncoderPtr computeEncoder = commandBuffer->createComputeEncoder();
    testImageGrayDraw(computeEncoder, computePipeline, renderTexture, computeTexture);
    computeEncoder->EndEncode();
    
    RenderEncoderPtr renderEncoder = commandBuffer->createDefaultRenderEncoder();
    testPost(renderEncoder, renderTexture);
    renderEncoder->EndEncode();
    commandBuffer->presentFrameBuffer();
}

@end
