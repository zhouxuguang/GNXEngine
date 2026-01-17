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
#include <algorithm>
#include <mutex>

NS_RENDERSYSTEM_BEGIN

SceneManager* SceneManager::GetInstance()
{
    static std::once_flag flag;
    static SceneManager *instance = nullptr;
    std::call_once(flag, []() {
        instance = new SceneManager();
    });
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
    
    // 默认创建延迟渲染器
    SetRenderPath(RenderPath::Deferred);
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

void SceneManager::RemoveLight(Light* light)
{
    if (!light)
    {
        return;
    }

    auto it = std::find(mLights.begin(), mLights.end(), light);
    if (it != mLights.end())
    {
        mLights.erase(it);
    }
}

void SceneManager::DestroyLight(Light* light)
{
    if (!light)
    {
        return;
    }

    RemoveLight(light);
    delete light;
}

void SceneManager::ClearLights()
{
    for (auto light : mLights)
    {
        delete light;
    }
    mLights.clear();
}

void SceneManager::ClearScene()
{
    // 清空所有灯光
    ClearLights();

    // 释放根节点及其所有子节点
    if (mRootSceneNode)
    {
        delete mRootSceneNode;
        mRootSceneNode = new SceneNode();
    }
}

void SceneManager::ResetScene()
{
    // 重置到初始状态
    ClearScene();

    // 重新创建天空盒和后处理
    if (mSkyBoxNode)
    {
        delete mSkyBoxNode;
        mSkyBoxNode = new SkyBoxNode();
    }

    if (mPostProcessing)
    {
        delete mPostProcessing;
        mPostProcessing = new PostProcessing(GetRenderDevice());
    }

    // 清空相机列表
    mCameras.clear();
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

void SceneManager::SetRenderPath(RenderPath path)
{
    mRenderPath = path;
}

void SceneManager::Render(RenderEncoderPtr renderEncoder)
{
    // 如果是延迟渲染，获取最终纹理并呈现
    if (mRenderPath == RenderPath::Deferred)
    {
        // TODO: 呈现最终纹理
        // RCTexturePtr finalTexture = mDeferredRenderer->GetFinalTexture();
        // PresentTexture(renderEncoder, finalTexture);
    }
    
    // 保留原有的渲染逻辑（前向渲染）
    if (!mRootSceneNode)
    {
        return;
    }

    RenderInfo renderInfo = GetRenderInfo();
    renderInfo.renderEncoder = renderEncoder;

    // 从根节点开始递归渲染
    RenderNodeRecursive(mRootSceneNode, renderInfo);

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

void SceneManager::RenderNodeRecursive(SceneNode* node, const RenderInfo& renderInfo)
{
    if (!node || !node->IsVisible() || !node->IsActive())
    {
        // 跳过不可见或不活跃的节点
        return;
    }

    // 2. 渲染当前节点
    if (node->GetAllAttachedObjects().size() > 0 || node->GetComponentCount() > 0)
    {
        // 使用缓存的 UBO，避免每帧创建新的
        UniformBufferPtr modelUniform = node->GetOrCreateModelUBO(GetRenderDevice());

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

    // 3. 递归渲染子节点（子节点会通过 mParentNode 获取父节点变换）
    for (SceneNode* child : node->GetAllNodes())
    {
        RenderNodeRecursive(child, renderInfo);
    }
}

void SceneManager::UpdateNodeRecursive(SceneNode* node, float deltaTime)
{
    if (!node || !node->IsActive())
    {
        // 跳过不活跃的节点
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
