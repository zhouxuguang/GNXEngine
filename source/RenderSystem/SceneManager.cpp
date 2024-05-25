//
//  SceneManager.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/13.
//

#include "SceneManager.h"
#include "RenderParameter.h"
#include "mesh/MeshRenderer.h"
#include "skinnedMesh/SkinnedMeshRenderer.h"
#include "animation/SkeletonAnimation.h"
#include "SkyBoxNode.h"

NS_RENDERSYSTEM_BEGIN

SceneManager* SceneManager::GetInstance()
{
    static SceneManager instance;
    return &instance;
}

SceneManager::SceneManager()
{
    mRootSceneNode = new SceneNode();
    mSkyBoxNode = new SkyBoxNode();
    
    mCameraUBO = getRenderDevice()->createUniformBufferWithSize(sizeof(cbPerCamera));
    mLightUBO = getRenderDevice()->createUniformBufferWithSize(sizeof(cbLighting));
}

SceneManager::~SceneManager()
{
    mRootSceneNode = nullptr;
}

SceneNode * SceneManager::getRootNode() const
{
    return mRootSceneNode;
}

Light * SceneManager::createLight(const std::string &name, Light::LightType type)
{
    Light *light = nullptr;
    switch (type)
    {
        case Light::PointLight:
            light = new PointLight(name);
            break;
            
        case Light::SpotLight:
            light = new SpotLight(name);
            break;
            
        case Light::DirectionLight:
            light = new DirectionLight(name);
            break;
            
        default:
            break;
    }
    if (light)
    {
        mLights.push_back(light);
    }
    
    return light;
}

Light * SceneManager::getLight(const std::string &name) const
{
    for (auto iter : mLights)
    {
        if (iter->getName() == name)
        {
            return iter;
        }
    }
    return nullptr;
}
 
bool SceneManager::hasLight(const std::string &name) const
{
    for (auto iter : mLights)
    {
        if (iter->getName() == name)
        {
            return true;
        }
    }
    return false;
}

CameraPtr SceneManager::createCamera(const std::string &name)
{
    CameraPtr camera = std::make_shared<Camera>(getRenderDevice()->getRenderDeviceType(), name);
    
    mCameraMani = new ArcballManipulate(camera);
    
    mCameras.push_back(camera);
    return camera;
}

CameraPtr SceneManager::getCamera(const std::string &name) const
{
    for (auto iter : mCameras)
    {
        if (iter->GetName() == name)
        {
            return iter;
        }
    }
    
    return nullptr;
}

void SceneManager::Render(RenderEncoderPtr renderEncoder)
{
    if (!mRootSceneNode)
    {
        return;
    }
    
    RenderInfo renderInfo = GetRenderInfo();
    renderInfo.renderEncoder = renderEncoder;
    
    for (const auto &iter : mRootSceneNode->GetAllNodes())
    {
        // 获得node的变换组件
        TransformComponent *transformCom = iter->QueryComponentT<TransformComponent>();
        assert(transformCom);
        
        UniformBufferPtr modelUniform = getRenderDevice()->createUniformBufferWithSize(sizeof(cbPerObject));
        cbPerObject modelMatrix;
        modelMatrix.MATRIX_M = transformCom->transform.TransformToMat4();
        modelMatrix.MATRIX_M_INV = transformCom->transform.Inverse().TransformToMat4();
        modelMatrix.MATRIX_Normal = modelMatrix.MATRIX_M.Transpose();
        modelUniform->setData(&modelMatrix, 0, sizeof(cbPerObject));
        
        renderInfo.objectUBO = modelUniform;
        
        //获得网格渲染组件
        MeshRenderer * meshRender = iter->QueryComponentT<MeshRenderer>();
        SkinnedMeshRenderer *skinnedMeshRenderer = iter->QueryComponentT<SkinnedMeshRenderer>();
        
        if (meshRender)
        {
            meshRender->Render(renderInfo);
        }
        else if (skinnedMeshRenderer)
        {
            SkeletonAnimation *skeAnimation = iter->QueryComponentT<SkeletonAnimation>();
            assert(skeAnimation);
            skinnedMeshRenderer->Render(renderInfo, skeAnimation->mCPUSkin);
        }
        
    }
    
    //天空盒最后绘制
    if (mSkyBoxNode)
    {
        mSkyBoxNode->Render(renderEncoder);
    }
}

void SceneManager::Update(float deltaTime)
{
    //相机操作器更新
    mCameraMani->Update();
    
    //更新相机
    CameraPtr cameraPtr = getCamera("MainCamera");
    if (!cameraPtr)
    {
        return;
    }
    cbPerCamera perCamera;
    perCamera.MATRIX_P = cameraPtr->GetProjectionMatrix();
    perCamera.MATRIX_V = cameraPtr->GetViewMatrix();
    mCameraUBO->setData(&perCamera, 0, sizeof(perCamera));
    
    //更新灯光
    Light * pointLight = getLight("mainLight");
    
    cbLighting lightInfo;
    Vector3f lightColor = pointLight->getColor();
    lightInfo.LightColor = mathutil::make_simd_float4(lightColor.x, lightColor.y, lightColor.z, 1.0);
    lightInfo.Strength = mathutil::make_simd_float3(pointLight->getStrength());
    Vector3f lightPos = pointLight->getPosition();
    lightInfo.WorldSpaceLightPos = mathutil::make_simd_float4(lightPos.x, lightPos.y, lightPos.z, 1.0);
    lightInfo.FalloffStart = pointLight->getFalloffStart();
    lightInfo.FalloffEnd = pointLight->getFalloffEnd();
    
    mLightUBO->setData(&lightInfo, 0, sizeof(cbLighting));
    
    for (const auto &iter : mRootSceneNode->GetAllNodes())
    {
        //获得网格渲染组件
//        SkinnedMeshRenderer *skinnedMeshRenderer = iter->QueryComponentT<SkinnedMeshRenderer>();
//        if (skinnedMeshRenderer)
//        {
//            skinnedMeshRenderer->Update(deltaTime);
//        }
        
        iter->Update(deltaTime);
    }
    
}

NS_RENDERSYSTEM_END
