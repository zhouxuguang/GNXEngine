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
#include "DeferredLightingPass.h"
#include "HiZPass.h"
#include "SSAOPass.h"
#include "MotionBlurPass.h"
#include "PostProcess/PostProcessing.h"
#include "SkyBoxNode.h"
#include "terrain/TerrainComponent.h"
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

    /**
     * 设置 IBL 环境贴图资源（由 Demo/场景层在初始化时调用）
     * @param irradianceMap  漫反射辐照度贴图 (CubeMap, 可为 nullptr 暂时禁用)
     * @param prefilteredMap 预过滤高光贴图 (CubeMap + Mipmap, 可为 nullptr)
     * @param brdfLUT        BRDF 分割求和预积分表 (2D Texture, 可为 nullptr)
     */
    void SetIBLTextures(RCTexturePtr irradianceMap,
                        RCTexturePtr prefilteredMap,
                        RCTexturePtr brdfLUT);

    /**
     * 设置运动模糊开关（默认关闭）
     */
    void SetMotionBlurEnabled(bool enabled);
    bool IsMotionBlurEnabled() const;

private:
    FrameGraphResource RenderPreDepthPass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const std::vector<DepthMeshItem>& meshItems,
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO,
        const std::vector<TerrainComponent*>& terrainItems = {},
        const mathutil::Frustumf& frustum = mathutil::Frustumf());

    GBufferData RenderBasePass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const std::vector<DepthMeshItem>& meshItems,
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO,
        FrameGraphResource preDepthTexture = -1,
        const std::vector<TerrainComponent*>& terrainItems = {},
        const mathutil::Frustumf& frustum = mathutil::Frustumf());

    void RenderPresentPass(FrameGraph& frameGraph, CommandBufferPtr commandBuffer, FrameGraphResource depthResource);

    /**
     * 执行Hi-Z生成Pass
     */
    HiZOutput BuildHiZPass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        FrameGraphResource depthTexture);

    /**
     * 执行延迟光照Pass
     */
    FrameGraphResource RenderDeferredLightingPass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const GBufferData& gbufferData,
        FrameGraphResource depthTexture,
        UniformBufferPtr cameraUBO,
        const HiZOutput& hiZOutput = HiZOutput(),
        const SSAOOutput& ssaoOutput = SSAOOutput());

    /**
     * 收集场景中的光源
     */
    void CollectLights(
        std::vector<DirectionLight*>& directionalLights,
        std::vector<PointLight*>& pointLights,
        std::vector<SpotLight*>& spotLights);

    /**
     * 执行前向渲染Pass（用于半透明物体）
     */
    void RenderForwardPass();

    /**
     * 执行天空盒Pass（在延迟光照之后、后处理之前）
     * 通过深度测试 LEQUAL 只填充远平面区域
     */
    FrameGraphResource RenderSkyboxPass(
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        FrameGraphResource colorTexture,
        FrameGraphResource depthTexture,
        UniformBufferPtr cameraUBO,
        SkyBoxNode* skyBoxNode);

    /**
     * 递归收集场景中的所有网格
     * @param node 场景节点
     * @param meshItems 静态网格列表
     * @param skinnedMeshItems 蒙皮网格列表
     * @param terrainItems 地形组件列表
     */
    void CollectMeshesRecursive(
        SceneNode* node,
        std::vector<DepthMeshItem>& meshItems,
        std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        std::vector<TerrainComponent*>& terrainItems);

    /**
     * 渲染场景（延迟渲染路径）
     * @param deltaTime 帧时间（秒）
     */
    virtual void Render(SceneManager *sceneManager, float deltaTime) override;
    
    void UpdateCameraView(SceneManager *sceneManager);

private:
    GBufferRendererPtr mGBufferRenderer;
    DepthRendererUniPtr mDepthRender = nullptr;
    DeferredLightingPassPtr mDeferredLightingPass = nullptr;
    HiZPassPtr mHiZPass = nullptr;
    SSAOPassPtr mSSAOPass = nullptr;
    MotionBlurPassPtr mMotionBlurPass = nullptr;
    PostProcessing* mPostProcessing = nullptr;
    bool mEnableMotionBlur = false;
    
    uint32_t mWidth = 1;
    uint32_t mHeight = 1;
    
    // Hi-Z输出资源（供后续Pass使用）
    HiZOutput mLastHiZOutput;

    // IBL 资源（由外部设置，每帧传递给延迟光照Pass）
    RCTexturePtr mIBLIrradianceMap = nullptr;     // 漫反射辐照度
    RCTexturePtr mIBLPrefilteredMap = nullptr;    // 预过滤高光
    RCTexturePtr mIBLBRDFLUT = nullptr;           // BRDF 预积分表
};

typedef std::shared_ptr<DeferredSceneRenderer> DeferredSceneRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_DEFERRED_SCENE_RENDERER_H
