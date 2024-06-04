//
//  SampleApplication.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#include "SampleApplication.h"

NS_RENDERSYSTEM_BEGIN

SampleApplication::SampleApplication(RenderDeviceType deviceType, ViewHandle nativeWindow)
{
    mRenderdevice = createRenderDevice(deviceType, nativeWindow);
}

SampleApplication::~SampleApplication()
{
    //
}

void SampleApplication::Init()
{
    //
}

void SampleApplication::Resize(uint32_t width, uint32_t height)
{
    if (mRenderdevice)
    {
        mRenderdevice->resize((uint32_t)width, (uint32_t)height);
    }
    mWidth = width;
    mHeight = height;
    
    //test
    TextureDescriptor textureDescriptor;
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.mipmaped = false;
    textureDescriptor.format = kTexFormatRGBA16Float;
    mRenderTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    mComputeTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
    textureDescriptor.width = width;
    textureDescriptor.height = height;
    textureDescriptor.mipmaped = false;
    textureDescriptor.format = kTexFormatDepth32FloatStencil8;
    mDepthStencilTexture = mRenderdevice->createRenderTexture(textureDescriptor);
    
    mSceneManager = SceneManager::GetInstance();
    SkyBox* skybox = nullptr;//initSky(mRenderdevice);
    mSceneManager->GetSkyBox()->AttachSkyBoxObject(skybox);
    //initPostResource(mRenderdevice);
    
    //初始化相机
    CameraPtr cameraPtr = mSceneManager->createCamera("MainCamera");
    cameraPtr->LookAt(mathutil::Vector3f(0, 0, 5), mathutil::Vector3f(0, 0, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, float(width) / height, 0.1f, 100.f);
    
    //初始化灯光信息
    Light * pointLight = mSceneManager->createLight("mainLight", Light::LightType::PointLight);
    pointLight->setColor(Vector3f(1.0, 1.0, 1.0));
    //pointLight->setPosition(Vector3f(5.0, 8.0, 0.0));
    pointLight->setPosition(Vector3f(-1.0, -1.0, -1.0));
    pointLight->setFalloffStart(5);
    pointLight->setFalloffEnd(300);
    pointLight->setStrength(Vector3f(8.0, 8.0, 8.0));
    
    Quaternionf rotate;
    rotate.FromAngleAxis(90, Vector3f(1.0, 0.0, 0.0));
    //sceneManager->getRootNode()->createRendererNode("hat", "DamagedHelmet/glTF/DamagedHelmet.gltf");
    
    //gltf/BrainStem/glTF
//    sceneManager->getRootNode()->createRendererNode("hat", "gltf/BrainStem/glTF/BrainStem.gltf",
//                                                    Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(3, 3, 3));
    mSceneManager->getRootNode()->createRendererNode("hat", "skin/Woman.gltf", Vector3f(0, -3.0, -2), Quaternionf(), Vector3f(0.01, 0.01, 0.01));
    
    mSceneManager->getRootNode()->createRendererNode("Marry", "asset/Marry.obj", Vector3f(0, -2.0, 0));
    
    //sceneManager->getRootNode()->createRendererNode("Marry", "nanosuit/nanosuit.obj", Vector3f(0, -4.0, 0));
    
    //TestADD();
    //computeTexture = TestImageGray();
    //computePipeline = initTestimageGray();
    
}

void SampleApplication::Render(float deltaTime)
{
    //
}

NS_RENDERSYSTEM_END
