//
//  DeferredSceneRenderer.h
//  GNXEngine
//
//  延迟渲染场景渲染器
//

#ifndef GNXENGINE_DEFERRED_SCENE_RENDERER_H
#define GNXENGINE_DEFERRED_SCENE_RENDERER_H

#include "SceneRenderer.h"
#include "GBufferRenderer.h"
#include "DepthRenderer.h"
#include "PostProcess/PostProcessing.h"
#include <vector>

NS_RENDERSYSTEM_BEGIN

// 前置声明
class SceneNode;

/**
 * 延迟渲染场景渲染器
 * 
 * 继承自SceneRenderer，实现延迟渲染路径
 * 负责管理G-Buffer的生成、延迟光照计算以及最终的图像合成
 */
class RENDERSYSTEM_API DeferredSceneRenderer : public SceneRenderer
{
public:
    DeferredSceneRenderer();
    ~DeferredSceneRenderer() override;
    
    /**
     * 设置G-Buffer配置
     */
    void SetGBufferConfig(const GBufferRenderer::GBufferConfig& config);
    const GBufferRenderer::GBufferConfig& GetGBufferConfig() const;

private:
    FrameGraphResource RenderPreDepthPass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const std::vector<DepthMeshItem>& meshItems,
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO);

    GBufferData RenderBasePass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const std::vector<DepthMeshItem>& meshItems,
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO);

    void RenderPresentPass(FrameGraph& frameGraph, CommandBufferPtr commandBuffer, FrameGraphResource depthResource);

    /**
     * 执行延迟光照Pass
     */
    void RenderDeferredLightingPass();

    /**
     * 执行前向渲染Pass（用于半透明物体）
     */
    void RenderForwardPass();

    /**
     * 递归收集场景中的所有网格
     * @param node 场景节点
     * @param meshItems 静态网格列表
     * @param skinnedMeshItems 蒙皮网格列表
     */
    void CollectMeshesRecursive(
        SceneNode* node,
        std::vector<DepthMeshItem>& meshItems,
        std::vector<DepthSkinnedMeshItem>& skinnedMeshItems);

    /**
     * 渲染场景（延迟渲染路径）
     * @param deltaTime 帧时间（秒）
     */
    virtual void Render(SceneManager *sceneManager, float deltaTime) override;

private:
    GBufferRendererPtr mGBufferRenderer;
    DepthRendererUniPtr mDepthRender = nullptr;
    PostProcessing* mPostProcessing = nullptr;
};

typedef std::shared_ptr<DeferredSceneRenderer> DeferredSceneRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_DEFERRED_SCENE_RENDERER_H
