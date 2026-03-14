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

//=============================================================================
// 深度渲染参数结构
//=============================================================================

/**
 * @brief 单个网格的深度渲染数据
 * 包含网格指针和对应的object UBO
 */
struct DepthMeshItem
{
    MeshPtr mesh;               // 网格指针
    UniformBufferPtr objectUBO; // 该网格的model矩阵UBO
};

/**
 * @brief 单个蒙皮网格的深度渲染数据
 * 包含蒙皮网格指针和对应的object UBO
 */
struct DepthSkinnedMeshItem
{
    SkinnedMeshPtr mesh;        // 蒙皮网格指针
    UniformBufferPtr objectUBO; // 该网格的model矩阵UBO
};

/**
 * @brief 深度渲染的网格数据
 * 包含静态网格和蒙皮网格
 */
struct DepthMeshData
{
    /** 静态网格列表（每个mesh带自己的objectUBO） */
    std::vector<DepthMeshItem> staticMeshes;
    
    /** 蒙皮网格列表（每个mesh带自己的objectUBO） */
    std::vector<DepthSkinnedMeshItem> skinnedMeshes;
    
    /** 获取总网格数量 */
    size_t GetTotalMeshCount() const
    {
        size_t count = 0;
        count += staticMeshes.size();
        count += skinnedMeshes.size();
        return count;
    }
    
    /** 是否有网格数据 */
    bool HasMeshes() const
    {
        return (!staticMeshes.empty() || !skinnedMeshes.empty());
    }
};

/**
 * @brief 深度渲染的 UBO 数据
 * 包含相机、骨骼矩阵等 uniform buffer
 * 注意：objectUBO已移至每个mesh单独携带
 */
struct DepthUniformData
{
    /** 相机 UBO（包含视图投影矩阵） */
    UniformBufferPtr cameraUBO = nullptr;
    
    /** 骨骼变换矩阵 UBO（仅用于蒙皮网格，可选） */
    UniformBufferPtr skinnedMatrixUBO = nullptr;
    
    /** 检查 UBO 是否有效 */
    bool IsValid() const
    {
        return cameraUBO != nullptr;
    }
};

/**
 * @brief 深度渲染的完整参数
 * 整合了网格数据和 UBO 数据
 */
struct DepthRenderParams
{
    /** 渲染目标宽度 */
    uint32_t width = 1024;
    
    /** 渲染目标高度 */
    uint32_t height = 1024;
    
    /** 网格数据 */
    DepthMeshData meshes;
    
    /** UBO 数据 */
    DepthUniformData uniforms;
    
    /**
     * 便捷构造函数（仅静态网格）
     * @param staticMeshItems 网格和对应的objectUBO列表
     * @param cameraUBO 相机UBO
     * @param skinnedMatrixUBO 骨骼矩阵UBO（可选）
     */
    static DepthRenderParams Create(
        const std::vector<DepthMeshItem>& staticMeshItems,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr skinnedMatrixUBO = nullptr)
    {
        DepthRenderParams params;
        params.meshes.staticMeshes = std::move(staticMeshItems);
        params.uniforms.cameraUBO = cameraUBO;
        params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
        return params;
    }
    
    /**
     * 便捷构造函数（包含蒙皮网格）
     * @param staticMeshItems 静态网格和对应的objectUBO列表
     * @param skinnedMeshItems 蒙皮网格和对应的objectUBO列表
     * @param cameraUBO 相机UBO
     * @param skinnedMatrixUBO 骨骼矩阵UBO（可选）
     */
    static DepthRenderParams Create(
        const std::vector<DepthMeshItem>& staticMeshItems,
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr skinnedMatrixUBO = nullptr)
    {
        DepthRenderParams params;
        params.meshes.staticMeshes = std::move(staticMeshItems);
        params.meshes.skinnedMeshes = std::move(skinnedMeshItems);
        params.uniforms.cameraUBO = cameraUBO;
        params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
        return params;
    }
    
    /**
     * 便捷构造函数（仅蒙皮网格）
     * @param skinnedMeshItems 蒙皮网格和对应的objectUBO列表
     * @param cameraUBO 相机UBO
     * @param skinnedMatrixUBO 骨骼矩阵UBO（可选）
     */
    static DepthRenderParams Create(
        const std::vector<DepthSkinnedMeshItem>& skinnedMeshItems,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr skinnedMatrixUBO)
    {
        DepthRenderParams params;
        params.meshes.skinnedMeshes = std::move(skinnedMeshItems);
        params.uniforms.cameraUBO = cameraUBO;
        params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
        return params;
    }
    
    /** 检查参数是否有效 */
    bool IsValid() const
    {
        return meshes.HasMeshes() && uniforms.IsValid();
    }
};

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
     * @brief 使用 FrameGraph 渲染深度图（推荐接口，使用结构化参数）
     * @param passName Pass 名称（用于调试）
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲
     * @param params 深度渲染参数（包含网格和 UBO 数据）
     * @return 深度纹理的 FrameGraph 资源 ID
     */
    FrameGraphResource Render(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const DepthRenderParams& params);

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
