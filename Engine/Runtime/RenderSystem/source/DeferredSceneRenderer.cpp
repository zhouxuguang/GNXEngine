//
//  DeferredSceneRenderer.cpp
//  GNXEngine
//
//  延迟渲染场景渲染器实现
//

#include "DeferredSceneRenderer.h"
#include "SceneManager.h"
#include "SceneNode.h"
#include "mesh/MeshRenderer.h"
#include "skinnedMesh/SkinnedMeshRenderer.h"
#include "SkyBoxNode.h"
#include "RenderParameter.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "FrameGraph/FrameGraphExecuteContext.h"
#include "BuildSetting.h"
#include <algorithm>
#include <functional>

NS_RENDERSYSTEM_BEGIN

DeferredSceneRenderer::DeferredSceneRenderer()
    : SceneRenderer()
{
    mGBufferRenderer = std::make_shared<GBufferRenderer>();
    mDepthRender = std::make_unique<DepthRenderer>(GetRenderDevice().get());
    mDeferredLightingPass = std::make_shared<DeferredLightingPass>();
    mHiZPass = std::make_shared<HiZPass>();
    
    mPostProcessing = new PostProcessing(GetRenderDevice());
}

DeferredSceneRenderer::~DeferredSceneRenderer()
{
}

void DeferredSceneRenderer::SetGBufferConfig(const GBufferRenderer::GBufferConfig& config)
{
    mGBufferRenderer->SetConfig(config);
}

const GBufferRenderer::GBufferConfig& DeferredSceneRenderer::GetGBufferConfig() const
{
    return mGBufferRenderer->GetConfig();
}

void DeferredSceneRenderer::Render(SceneManager *sceneManager, float deltaTime)
{
    if (!sceneManager)
    {
        return;
    }
    
    UpdateCameraView(sceneManager);

    CommandQueuePtr graphicsQueue = RenderCore::GetRenderDevice()->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }

    // 创建FrameGraph执行上下文，包含命令缓冲区
    FrameGraphExecuteContext executeContext;
    executeContext.commandBuffer = commandBuffer;
    // Vulkan后端会利用commandBuffer自动处理layout转换

    // 创建FrameGraph
    FrameGraph frameGraph;

    // ========== 统一收集场景数据（只收集一次）==========
    std::vector<DepthMeshItem> meshItems;
    std::vector<DepthSkinnedMeshItem> skinnedMeshItems;
    
    SceneNode* rootNode = sceneManager->GetRootNode();
    if (rootNode)
    {
        CollectMeshesRecursive(rootNode, meshItems, skinnedMeshItems);
    }

    // 获取相机UBO
    RenderInfo renderInfo = sceneManager->GetRenderInfo();
    UniformBufferPtr cameraUBO = renderInfo.cameraUBO;

    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // ========== 执行渲染 Pass ==========
    
    // PreZPass
    FrameGraphResource depthResource = RenderPreDepthPass(
        frameGraph, commandBuffer, meshItems, skinnedMeshItems, cameraUBO);

    // Hi-Z Pass（在PreDepth之后、BasePass之前）
    HiZOutput hiZOutput;
    if (depthResource != -1)
    {
        hiZOutput = BuildHiZPass(frameGraph, commandBuffer, depthResource);
        mLastHiZOutput = hiZOutput;
    }

    // BasePass (G-Buffer)
    GBufferData gbufferData = RenderBasePass(
        frameGraph, commandBuffer, meshItems, skinnedMeshItems, cameraUBO, depthResource);

    // Deferred Lighting Pass
    FrameGraphResource lightingResult = RenderDeferredLightingPass(
        frameGraph, commandBuffer, gbufferData, depthResource, cameraUBO, hiZOutput);

    RenderPresentPass(frameGraph, commandBuffer, lightingResult);

    frameGraph.Compile();
    // 执行FrameGraph，RHI层会自动处理资源状态转换
    frameGraph.Execute(&executeContext, mTransientResources);

    return;

    // 阶段1: G-Buffer Pass

    // 阶段2: 延迟光照Pass

    // 阶段3: 渲染天空盒

    // 阶段4: 前向渲染Pass（半透明物体）
    RenderForwardPass();
}

void DeferredSceneRenderer::UpdateCameraView(SceneManager *sceneManager)
{
    uint32_t width = 1;
    uint32_t height = 1;
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (cameraPtr)
    {
        Vector2i viewSize = cameraPtr->GetViewSize();
        width = viewSize.x;
        height = viewSize.y;
    }
    mWidth = width;
    mHeight = height;
}

FrameGraphResource DeferredSceneRenderer::RenderPreDepthPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const std::vector<DepthMeshItem>& meshItems,
    const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
    UniformBufferPtr cameraUBO)
{
    FrameGraphResource depthResource = -1;

    if (meshItems.empty() && skinnedMeshItems.empty())
    {
        return depthResource;
    }

    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // 构建 DepthRenderParams
    DepthRenderParams params;
    params.width = mWidth;
    params.height = mHeight;
    params.meshes.staticMeshes = meshItems;
    params.meshes.skinnedMeshes = skinnedMeshItems;
    params.uniforms.cameraUBO = cameraUBO;
    params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;

    // 使用FrameGraph渲染深度图
    depthResource = mDepthRender->Render("DepthPass", frameGraph, commandBuffer, params);
    
    return depthResource;
}

GBufferData DeferredSceneRenderer::RenderBasePass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const std::vector<DepthMeshItem>& meshItems,
    const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
    UniformBufferPtr cameraUBO,
    FrameGraphResource preDepthTexture)
{
    // 收集蒙皮网格的骨骼矩阵UBO
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    if (!skinnedMeshItems.empty())
    {
        skinnedMatrixUBO = skinnedMeshItems[0].mesh->GetSkinnedMatrixBuffer();
    }

    // 构建 GBuffer 渲染参数
    GBufferRenderParams params;
    params.meshes.staticMeshes = meshItems;
    params.meshes.skinnedMeshes = skinnedMeshItems;
    params.uniforms.cameraUBO = cameraUBO;
    params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
    params.preDepthTexture = preDepthTexture;  // 传递 PreDepth 深度图

    // 初始化 GBufferRenderer（如果需要）
    if (!mGBufferRenderer->IsInitialized())
    {
        mGBufferRenderer->Initialize(mWidth, mHeight);
    }

    // 调用 GBufferRenderer::AddToFrameGraph
    return mGBufferRenderer->AddToFrameGraph("BasePass", frameGraph, commandBuffer, params);
}

HiZOutput DeferredSceneRenderer::BuildHiZPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    FrameGraphResource depthTexture)
{
    // 初始化Hi-Z Pass（如果需要）
    if (!mHiZPass->IsInitialized())
    {
        mHiZPass->Initialize();
    }

    // 构建Hi-Z参数
    HiZParams params;
    params.width = mWidth;
    params.height = mHeight;
    params.depthTexture = depthTexture;
    params.useReverseZ = BuildSetting::mUseReverseZ;

    // 添加Hi-Z Pass到FrameGraph
    return mHiZPass->AddToFrameGraph("HiZPass", frameGraph, commandBuffer, params);
}

void DeferredSceneRenderer::RenderPresentPass(FrameGraph& frameGraph, CommandBufferPtr commandBuffer, FrameGraphResource depthResource)
{
    frameGraph.AddPass("PresentPass",
    [=](RenderSystem::FrameGraph::Builder &builder, RenderSystem::FrameGraph::NoData &data)
    {
        builder.Read(depthResource, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);

        // present的pass必须设置这个标记，要不然不会执行
        builder.SetSideEffect();
    },
    [=](const RenderSystem::FrameGraph::NoData &data, RenderSystem::FrameGraphPassResources &resources, void *)
    {
        RenderSystem::FrameGraphTexture &colorTexture = resources.Get<RenderSystem::FrameGraphTexture>(depthResource);
        
        float color[4] = {1.0, 0.0, 0.0, 1.0};
        SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), color);

        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        
        mPostProcessing->SetRenderTexture(colorTexture.texture);
        mPostProcessing->Process(renderEncoder);
        
        renderEncoder->EndEncode();
        commandBuffer->PresentFrameBuffer();
    });
}

FrameGraphResource DeferredSceneRenderer::RenderDeferredLightingPass(
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const GBufferData& gbufferData,
    FrameGraphResource depthTexture,
    UniformBufferPtr cameraUBO,
    const HiZOutput& hiZOutput)
{
    // 初始化延迟光照Pass（如果需要）
    if (!mDeferredLightingPass->IsInitialized())
    {
        DeferredLightingConfig config;
        config.enableSSAO = false;
        config.enableSSR = false;
        config.enableTiledLighting = false;
        mDeferredLightingPass->Initialize(config);
    }

    // 收集场景中的光源
    std::vector<DirectionLight*> directionalLights;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    CollectLights(directionalLights, pointLights, spotLights);

    // 构建延迟光照参数
    DeferredLightingParams params;
    params.gBufferA = gbufferData.gBufferA;
    params.gBufferB = gbufferData.gBufferB;
    params.gBufferC = gbufferData.gBufferC;
    params.gBufferD = gbufferData.gBufferD;
    params.depthTexture = depthTexture;
    params.directionalLights = directionalLights;
    params.pointLights = pointLights;
    params.spotLights = spotLights;
    params.cameraUBO = cameraUBO;
    params.width = mWidth;
    params.height = mHeight;
    
    // 传递Hi-Z资源（如果可用）
    // 注意：这里需要在DeferredLightingParams中添加hiZTexture字段
    // 现在先预留接口，等实现SSR时使用
    // params.hiZTexture = hiZOutput.hiZTexture;

    // 执行延迟光照Pass
    DeferredLightingOutput output = mDeferredLightingPass->AddToFrameGraph(
        "DeferredLightingPass", frameGraph, commandBuffer, params);

    return output.lightingResult;
}

void DeferredSceneRenderer::CollectLights(
    std::vector<DirectionLight*>& directionalLights,
    std::vector<PointLight*>& pointLights,
    std::vector<SpotLight*>& spotLights)
{
    SceneManager* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }

    // 从 SceneManager 获取所有光源
    const std::vector<Light*>& allLights = sceneManager->GetAllLights();

    // 按类型分类
    for (Light* light : allLights)
    {
        if (!light)
        {
            continue;
        }

        switch (light->getLightType())
        {
        case Light::LightType::DirectionLight:
            directionalLights.push_back(static_cast<DirectionLight*>(light));
            break;
        case Light::LightType::PointLight:
            pointLights.push_back(static_cast<PointLight*>(light));
            break;
        case Light::LightType::SpotLight:
            spotLights.push_back(static_cast<SpotLight*>(light));
            break;
        }
    }
}

void DeferredSceneRenderer::RenderForwardPass()
{
    auto* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }
}

void DeferredSceneRenderer::CollectMeshesRecursive(
    SceneNode* node,
    std::vector<DepthMeshItem>& meshItems,
    std::vector<DepthSkinnedMeshItem>& skinnedMeshItems)
{
    if (!node || !node->IsVisible())
    {
        return;
    }
    
    RenderCore::RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();

    // 查询MeshRenderer组件（静态网格）
    MeshRenderer* meshRenderer = node->QueryComponentT<MeshRenderer>();
    if (meshRenderer)
    {
        MeshPtr mesh = meshRenderer->GetSharedMesh();
        if (mesh)
        {
            DepthMeshItem item;
            item.mesh = mesh;
            item.objectUBO = node->GetOrCreateModelUBO(renderDevice);
            meshItems.push_back(item);
        }
    }

    // 查询SkinnedMeshRenderer组件（蒙皮网格）
    SkinnedMeshRenderer* skinnedMeshRenderer = node->QueryComponentT<SkinnedMeshRenderer>();
    if (skinnedMeshRenderer)
    {
        SkinnedMeshPtr mesh = skinnedMeshRenderer->GetSharedMesh();
        if (mesh)
        {
            DepthSkinnedMeshItem item;
            item.mesh = mesh;
            item.objectUBO = node->GetOrCreateModelUBO(renderDevice);
            skinnedMeshItems.push_back(item);
        }
    }

    // 递归处理子节点
    const auto& children = node->GetAllNodes();
    for (SceneNode* child : children)
    {
        CollectMeshesRecursive(child, meshItems, skinnedMeshItems);
    }
}

NS_RENDERSYSTEM_END
