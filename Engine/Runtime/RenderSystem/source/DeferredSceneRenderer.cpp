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
#include <algorithm>

NS_RENDERSYSTEM_BEGIN

DeferredSceneRenderer::DeferredSceneRenderer()
    : SceneRenderer()
{
    mGBufferRenderer = std::make_shared<GBufferRenderer>();
    mDepthRender = std::make_unique<DepthRenderer>(GetRenderDevice().get());
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

void DeferredSceneRenderer::Render(float deltaTime)
{
    // 阶段1: G-Buffer Pass
    RenderGBufferPass();
    
    // 阶段2: 延迟光照Pass
    RenderDeferredLightingPass();
    
    // 阶段3: 渲染天空盒
    
    // 阶段4: 前向渲染Pass（半透明物体）
    RenderForwardPass();
}

void DeferredSceneRenderer::RenderGBufferPass()
{
    auto* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }
    
    // 开始G-Buffer渲染
    mGBufferRenderer->BeginGBufferPass();
    
    // 获取渲染信息
    RenderInfo renderInfo = sceneManager->GetRenderInfo();
    
    // TODO: 设置RenderEncoder
    // RenderEncoderPtr renderEncoder = ...;
    // renderInfo.renderEncoder = renderEncoder;
    
    // 递归渲染场景节点（G-Buffer）
    SceneNode* rootNode = sceneManager->GetRootNode();
    if (rootNode)
    {
        //RenderGBufferNodeRecursive(rootNode, renderInfo);
    }
    
    // 结束G-Buffer渲染
    mGBufferRenderer->EndGBufferPass();
}

void DeferredSceneRenderer::RenderDeferredLightingPass()
{
    // 执行延迟光照
    mGBufferRenderer->ExecuteDeferredLighting();
}

void DeferredSceneRenderer::RenderForwardPass()
{
    auto* sceneManager = SceneManager::GetInstance();
    if (!sceneManager)
    {
        return;
    }
}

RCTexturePtr DeferredSceneRenderer::GetGBufferTexture(uint32_t index) const
{
    return mGBufferRenderer->GetGBufferTexture(index);
}

RCTexturePtr DeferredSceneRenderer::GetFinalTexture() const
{
    return mGBufferRenderer->GetFinalTexture();
}

NS_RENDERSYSTEM_END
