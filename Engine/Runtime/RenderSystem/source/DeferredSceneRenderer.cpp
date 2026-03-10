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
#include <algorithm>

NS_RENDERSYSTEM_BEGIN

DeferredSceneRenderer::DeferredSceneRenderer()
    : SceneRenderer()
{
    mGBufferRenderer = std::make_shared<GBufferRenderer>();
    mDepthRender = std::make_unique<DepthRenderer>(GetRenderDevice().get());
    
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

    // BasePass (G-Buffer)
    GBufferData gbufferData = RenderBasePass(
        frameGraph, commandBuffer, meshItems, skinnedMeshItems, cameraUBO);

    RenderPresentPass(frameGraph, commandBuffer, depthResource);

    frameGraph.Compile();
    // 执行FrameGraph，RHI层会自动处理资源状态转换
    frameGraph.Execute(&executeContext, mTransientResources);

    return;

    // 阶段1: G-Buffer Pass

    // 阶段2: 延迟光照Pass
    RenderDeferredLightingPass();

    // 阶段3: 渲染天空盒

    // 阶段4: 前向渲染Pass（半透明物体）
    RenderForwardPass();
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
    UniformBufferPtr cameraUBO)
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

    // 初始化 GBufferRenderer（如果需要）
    if (!mGBufferRenderer->IsInitialized())
    {
        uint32_t width = 1920;   // TODO: 从配置获取
        uint32_t height = 1080;
        mGBufferRenderer->Initialize(width, height);
    }

    // 调用 GBufferRenderer::AddToFrameGraph
    return mGBufferRenderer->AddToFrameGraph("BasePass", frameGraph, commandBuffer, params);
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

void DeferredSceneRenderer::RenderDeferredLightingPass()
{
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
