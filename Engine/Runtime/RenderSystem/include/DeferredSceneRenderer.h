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
#include <memory>

NS_RENDERSYSTEM_BEGIN

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
    
    /**
     * 获取G-Buffer渲染器（用于调试或其他用途）
     */
    GBufferRendererPtr GetGBufferRenderer() const { return mGBufferRenderer; }
    
    /**
     * 渲染场景（延迟渲染路径）
     * @param deltaTime 帧时间（秒）
     */
    void Render(float deltaTime) override;
    
    /**
     * 设置是否使用FrameGraph
     */
    void SetUseFrameGraph(bool use) { mUseFrameGraph = use; }
    bool GetUseFrameGraph() const { return mUseFrameGraph; }
    
    /**
     * 获取G-Buffer纹理（用于调试）
     */
    RCTexturePtr GetGBufferTexture(uint32_t index) const;
    
    /**
     * 获取最终渲染结果
     */
    RCTexturePtr GetFinalTexture() const;

private:
    /**
     * 执行G-Buffer Pass
     */
    void RenderGBufferPass();
    
    /**
     * 执行延迟光照Pass
     */
    void RenderDeferredLightingPass();
    
    /**
     * 执行前向渲染Pass（用于半透明物体）
     */
    void RenderForwardPass();

private:
    GBufferRendererPtr mGBufferRenderer;
    
    bool mUseFrameGraph = false;
};

typedef std::shared_ptr<DeferredSceneRenderer> DeferredSceneRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_DEFERRED_SCENE_RENDERER_H
