//
//  FeedbackRenderer.h
//  GNXEngine
//
//  虚拟纹理反馈渲染器
//  降分辨率重绘使用 VT 材质的物体，将 page 编码写入 R32Uint feedback target。
//

#ifndef GNXENGINE_RENDERSYSTEM_FEEDBACK_RENDERER_H
#define GNXENGINE_RENDERSYSTEM_FEEDBACK_RENDERER_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "mesh/Mesh.h"
#include "mesh/MeshDrawUtil.h"
#include "skinnedMesh/SkinnedMesh.h"
#include "terrain/TerrainComponent.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

/**
 * @brief 单个网格的 feedback 渲染数据
 */
struct FeedbackMeshItem
{
    MeshPtr mesh;
    UniformBufferPtr objectUBO;
    std::vector<MaterialPtr> materials;
};

/**
 * @brief feedback 渲染参数
 */
struct FeedbackRenderParams
{
    uint32_t width  = 128;
    uint32_t height = 128;

    std::vector<FeedbackMeshItem> staticMeshes;
    std::vector<TerrainComponent*> terrainItems;

    UniformBufferPtr cameraUBO = nullptr;

    bool IsValid() const
    {
        return (!staticMeshes.empty() || !terrainItems.empty()) && cameraUBO != nullptr;
    }
};

/**
 * @brief 虚拟纹理反馈渲染器
 *
 * 在降分辨率下重绘使用 VT 材质的物体，PS 输出编码后的 page 信息到 R32Uint target。
 * 输出由 VirtualTextureFeedback::ReadbackAndDecode() 读取并解析为 page 请求。
 */
class RENDERSYSTEM_API FeedbackRenderer
{
public:
    FeedbackRenderer(RenderDevicePtr device);
    ~FeedbackRenderer();

    bool Initialize();
    void Shutdown();

    bool IsInitialized() const { return mInitialized; }

    /**
     * @brief 渲染 feedback pass
     * @param passName Pass 名称
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲
     * @param params 渲染参数（降分辨率尺寸 + VT 网格列表）
     * @param externalFeedbackTexture 外部的 feedback RT（来自 VirtualTextureFeedback）
     * @return feedback 纹理的 FrameGraph 资源 ID
     */
    FrameGraphResource Render(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const FeedbackRenderParams& params,
        RCTexturePtr externalFeedbackTexture,
        RCTexturePtr externalFeedbackDepth);

private:
    RenderDevicePtr mDevice = nullptr;
    GraphicsPipelinePtr mFeedbackPipeline = nullptr;
    GraphicsPipelinePtr mTerrainFeedbackPipeline = nullptr;
    bool mInitialized = false;
};

using FeedbackRendererPtr = std::shared_ptr<FeedbackRenderer>;
using FeedbackRendererUniPtr = std::unique_ptr<FeedbackRenderer>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_FEEDBACK_RENDERER_H */
