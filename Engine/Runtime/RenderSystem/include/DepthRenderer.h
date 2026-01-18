//
//  DepthRenderer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/11.
//  深度渲染工具类 - 用于生成ShadowMap和Depth Pre-Pass
//

#ifndef GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H
#define GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/MathUtil/include/AABB.h"
#include "Camera.h"
#include "Light.h"
#include "Scene.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "mesh/Mesh.h"
#include "skinnedMesh/SkinnedMesh.h"
#include "mesh/MeshDrawUtil.h"
#include <memory>
#include <vector>
#include <limits>
#include <unordered_map>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

// 前置声明
class FrameGraph;

/**
 * @brief 深度渲染器配置参数
 */
struct DepthRenderConfig
{
    uint32_t width = 1024;              // 深度纹理宽度
    uint32_t height = 1024;             // 深度纹理高度
    TextureFormat depthFormat = kTexFormatDepth32Float;  // 深度格式
};

/**
 * @brief 深度渲染器类
 *
 * 负责生成各种深度图像：
 * - ShadowMap：用于阴影渲染
 * - Depth Pre-Pass：用于提前剔除不可见像素，提升性能
 *
 */
class RENDERSYSTEM_API DepthRenderer
{
public:
    DepthRenderer(RenderDevice* device);
    ~DepthRenderer();

    /**
     * @brief 初始化深度渲染器
     * @param config 配置参数
     */
    bool Initialize(const DepthRenderConfig& config);

    /**
     * @brief 关闭深度渲染器，释放资源
     */
    void Shutdown();
    
    /**
     * @brief 使用 FrameGraph 渲染深度图
     * @param frameGraph 帧图
     * @param meshes 要渲染的静态网格列表
     * @param skinnedMeshes 要渲染的蒙皮网格列表
     * @param cameraUBO 相机 UBO
     * @param objectUBO 物体变换 UBO
     * @param skinnedMatrixUBO 骨骼矩阵 UBO（可选）
     * @return 深度纹理的 FrameGraph 资源 ID
     */
    FrameGraphResource Render(
        const std::string& passName,
        FrameGraph& frameGraph,
        const std::vector<MeshPtr>& meshes,
        const std::vector<SkinnedMeshPtr>& skinnedMeshes,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr objectUBO,
        UniformBufferPtr skinnedMatrixUBO = nullptr);
    
    /**
     * @brief 使用 FrameGraph 渲染深度图（简化版本，只渲染静态网格）
     * @param frameGraph 帧图
     * @param meshes 要渲染的网格列表
     * @param cameraUBO 相机 UBO
     * @param objectUBO 物体变换 UBO
     * @return 深度纹理的 FrameGraph 资源 ID
     */
    FrameGraphResource Render(
        const std::string& passName,
        FrameGraph& frameGraph,
        const std::vector<MeshPtr>& meshes,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr objectUBO);

    /**
     * @brief 更新配置参数
     * @param config 新的配置参数
     */
    void UpdateConfig(const DepthRenderConfig& config);

    /**
     * @brief 获取当前配置
     * @return 配置参数
     */
    const DepthRenderConfig& GetConfig() const;

    /**
     * @brief 是否已初始化
     * @return 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }

private:
    RenderDevice* mDevice;
    DepthRenderConfig mConfig;
    
    // 深度渲染 PSO
    GraphicsPipelinePtr mDepthOnlyPipeline = nullptr;
    GraphicsPipelinePtr mSkinnedDepthOnlyPipeline = nullptr;
    
    // FrameGraph 资源 ID
    FrameGraphResource mDepthResource;
    
    bool mInitialized = false;
};

using DepthRendererPtr = std::shared_ptr<DepthRenderer>;
using DepthRendererUniPtr = std::unique_ptr<DepthRenderer>;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H */
