//
//  GBufferRenderer.h
//  GNXEngine
//
//  G-Buffer渲染器 - 管理延迟渲染的G-Buffer生成和光照计算
//

#ifndef GNXENGINE_GBUFFER_RENDERER_H
#define GNXENGINE_GBUFFER_RENDERER_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "FrameGraph/GraphNode.h"
#include "DepthRenderer.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

// 前置声明
class Material;
class SceneRenderer;
class FrameGraph;

struct GBufferData
{
    FrameGraphResource depth;
	FrameGraphResource sceneColor;
    FrameGraphResource gBufferA;
    FrameGraphResource gBufferB;
    FrameGraphResource gBufferC;
};

// G-Buffer 渲染的网格数据
struct GBufferMeshData
{
    std::vector<DepthMeshItem> staticMeshes;
    std::vector<DepthSkinnedMeshItem> skinnedMeshes;
};

// G-Buffer 渲染的 UBO 数据
struct GBufferUniformData
{
    UniformBufferPtr cameraUBO = nullptr;
    UniformBufferPtr skinnedMatrixUBO = nullptr;
};

// G-Buffer 渲染的完整参数
struct GBufferRenderParams
{
    GBufferMeshData meshes;
    GBufferUniformData uniforms;
    
    static GBufferRenderParams Create(
        const std::vector<DepthMeshItem>& staticMeshItems,
        UniformBufferPtr cameraUBO,
        UniformBufferPtr skinnedMatrixUBO = nullptr)
    {
        GBufferRenderParams params;
        params.meshes.staticMeshes = std::move(staticMeshItems);
        params.uniforms.cameraUBO = cameraUBO;
        params.uniforms.skinnedMatrixUBO = skinnedMatrixUBO;
        return params;
    }
};

class GBufferRenderer
{
public:
    GBufferRenderer();
    ~GBufferRenderer();
    
    // 初始化G-Buffer渲染器
    bool Initialize(uint32_t width, uint32_t height);
    
    // 重置大小（窗口大小改变时调用）
    void Resize(uint32_t width, uint32_t height);
    
    // 设置当前材质
    void SetMaterial(std::shared_ptr<Material> material);
    
    // 添加到FrameGraph
    GBufferData AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const GBufferRenderParams& params);
    
    // 获取FrameGraph资源ID（用于调试或FrameGraph集成）
    struct FrameGraphResourceIds
    {
        FrameGraphResource depth;
        FrameGraphResource gBuffer0;
        FrameGraphResource gBuffer1;
        FrameGraphResource gBuffer2;
        FrameGraphResource gBuffer3;  // 可选
        FrameGraphResource finalColor;
    };
    
    // G-Buffer配置
    struct GBufferConfig
    {
        bool enablePositionTexture = false;  // 是否使用独立的位置纹理（false则从深度重建）
        bool useOctahedralNormal = true;   // 使用octahedral法线编码
        uint32_t msaaSampleCount = 1;      // MSAA采样数
    };
    
    void SetConfig(const GBufferConfig& config) { mConfig = config; }
    const GBufferConfig& GetConfig() const { return mConfig; }
    
private:
    
    // 创建G-Buffer渲染管线
    void CreateGBufferPipeline();

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    GBufferConfig mConfig;
    
    // 当前材质
    std::shared_ptr<Material> mCurrentMaterial = nullptr;
    // 渲染管线
    GraphicsPipelinePtr mGBufferPipeline = nullptr;
    bool mIsInitialized = false;
};

typedef std::shared_ptr<GBufferRenderer> GBufferRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_GBUFFER_RENDERER_H
