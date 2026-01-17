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
#include "Runtime/MathUtil/include/Matrix4x4.h"

NS_RENDERSYSTEM_BEGIN

SceneManager* SceneManager::GetInstance()
{
    static SceneManager *instance = nullptr;
    if (nullptr == instance)
    {
        instance = new SceneManager();
    }
    return instance;
}

bool SceneManager::HasCamera(const std::string &name) const
{
    for (const auto& iter : mCameras)
    {
        if (iter->GetName() == name)
        {
            return true;
        }
    }
    return false;
}

SceneManager::SceneManager()
{
    mRootSceneNode = new SceneNode();
    mSkyBoxNode = new SkyBoxNode();
    mPostProcessing = new PostProcessing(GetRenderDevice());
    
    mCameraUBO = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbPerCamera));
    mLightUBO = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbLighting));
}

SceneManager::~SceneManager()
{
    // 释放相机操作器
    if (mCameraMani)
    {
        delete mCameraMani;
        mCameraMani = nullptr;
    }

    // 释放后处理
    if (mPostProcessing)
    {
        delete mPostProcessing;
        mPostProcessing = nullptr;
    }

    // 释放天空盒
    if (mSkyBoxNode)
    {
        delete mSkyBoxNode;
        mSkyBoxNode = nullptr;
    }

    // 释放根节点（递归删除所有子节点）
    if (mRootSceneNode)
    {
        delete mRootSceneNode;
        mRootSceneNode = nullptr;
    }

    // 释放所有灯光
    for (auto light : mLights)
    {
        delete light;
    }
    mLights.clear();
}

SceneNode * SceneManager::GetRootNode() const
{
    return mRootSceneNode;
}

Light * SceneManager::CreateLight(const std::string &name, Light::LightType type)
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

Light * SceneManager::GetLight(const std::string &name) const
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

bool SceneManager::HasLight(const std::string &name) const
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

CameraPtr SceneManager::CreateCamera(const std::string &name)
{
    CameraPtr camera = std::make_shared<Camera>(GetRenderDevice()->GetRenderDeviceType(), name);

    // 释放旧的相机操作器，避免内存泄漏
    if (mCameraMani)
    {
        delete mCameraMani;
        mCameraMani = nullptr;
    }

    mCameraMani = new ArcballManipulate(camera);

    mCameras.push_back(camera);
    return camera;
}

CameraPtr SceneManager::GetCamera(const std::string &name) const
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

    // 从根节点开始递归渲染，使用默认构造函数创建单位矩阵作为初始父变换
    mathutil::Matrix4x4f identityMatrix;
    RenderNodeRecursive(mRootSceneNode, identityMatrix, renderInfo);

    // 天空盒最后绘制
    if (mSkyBoxNode)
    {
        mSkyBoxNode->Render(renderEncoder);
    }

    if (mPostProcessing)
    {
        //mPostProcessing->Process(renderEncoder);
    }
}

void SceneManager::Update(float deltaTime)
{
    //相机操作器更新
    if (mCameraMani)
    {
        //mCameraMani->Update();
    }
    
    //更新相机 - 支持当前激活的相机
    CameraPtr cameraPtr = GetCamera("MainCamera");
    if (!cameraPtr && !mCameras.empty())
    {
        // 如果找不到 MainCamera，使用第一个相机
        cameraPtr = mCameras[0];
    }

    if (!cameraPtr)
    {
        // 递归更新所有节点
        UpdateNodeRecursive(mRootSceneNode, deltaTime);
        return;
    }

    cbPerCamera perCamera;
    perCamera.MATRIX_P = cameraPtr->GetProjectionMatrix();
    perCamera.MATRIX_V = cameraPtr->GetViewMatrix();
    mCameraUBO->SetData(&perCamera, 0, sizeof(perCamera));

    //更新灯光 - 支持第一个有效灯光
    Light * pointLight = GetLight("mainLight");
    if (!pointLight && !mLights.empty())
    {
        // 如果找不到 mainLight，使用第一个灯光
        pointLight = mLights[0];
    }

    if (pointLight)
    {
        cbLighting lightInfo;
        Vector3f lightColor = pointLight->getColor();
        lightInfo.LightColor = mathutil::make_simd_float4(lightColor.x, lightColor.y, lightColor.z, 1.0);
        lightInfo.Strength = mathutil::make_simd_float3(pointLight->getStrength());
        Vector3f lightPos = pointLight->getPosition();
        lightInfo.WorldSpaceLightPos = mathutil::make_simd_float4(lightPos.x, lightPos.y, lightPos.z, 1.0);
        lightInfo.FalloffStart = pointLight->getFalloffStart();
        lightInfo.FalloffEnd = pointLight->getFalloffEnd();

        mLightUBO->SetData(&lightInfo, 0, sizeof(cbLighting));
    }

    // 递归更新所有节点
    UpdateNodeRecursive(mRootSceneNode, deltaTime);

}

void SceneManager::RenderNodeRecursive(SceneNode* node, const mathutil::Matrix4x4f& parentWorldMatrix, const RenderInfo& renderInfo)
{
    if (!node)
    {
        return;
    }

    // 1. 计算当前节点的世界矩阵
    mathutil::Matrix4x4f currentWorldMatrix = parentWorldMatrix;
    TransformComponent* transformCom = node->QueryComponentT<TransformComponent>();

    if (transformCom)
    {
        mathutil::Matrix4x4f localMatrix = transformCom->transform.TransformToMat4();
        currentWorldMatrix = parentWorldMatrix * localMatrix;  // 关键：父矩阵 × 本地矩阵
    }

    // 2. 渲染当前节点
    if (node->GetAllAttachedObjects().size() > 0 || node->GetComponentCount() > 0)
    {
        UniformBufferPtr modelUniform = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbPerObject));
        cbPerObject modelMatrix;
        modelMatrix.MATRIX_M = currentWorldMatrix;
        modelMatrix.MATRIX_M_INV = currentWorldMatrix.Inverse();
        modelMatrix.MATRIX_Normal = currentWorldMatrix.Transpose();
        modelUniform->SetData(&modelMatrix, 0, sizeof(cbPerObject));

        RenderInfo nodeRenderInfo = renderInfo;
        nodeRenderInfo.objectUBO = modelUniform;

        MeshRenderer* meshRender = node->QueryComponentT<MeshRenderer>();
        SkinnedMeshRenderer* skinnedMeshRenderer = node->QueryComponentT<SkinnedMeshRenderer>();

        if (meshRender)
        {
            meshRender->Render(nodeRenderInfo);
        }
        else if (skinnedMeshRenderer)
        {
            SkeletonAnimation* skeAnimation = node->QueryComponentT<SkeletonAnimation>();
            if (skeAnimation)
            {
                skinnedMeshRenderer->Render(nodeRenderInfo, skeAnimation->mCPUSkin);
            }
        }
    }

    // 3. 递归渲染子节点
    for (SceneNode* child : node->GetAllNodes())
    {
        RenderNodeRecursive(child, currentWorldMatrix, renderInfo);
    }
}

void SceneManager::UpdateNodeRecursive(SceneNode* node, float deltaTime)
{
    if (!node)
    {
        return;
    }

    // 更新当前节点
    node->Update(deltaTime);

    // 递归更新所有子节点
    for (SceneNode* child : node->GetAllNodes())
    {
        UpdateNodeRecursive(child, deltaTime);
    }
}

NS_RENDERSYSTEM_END
