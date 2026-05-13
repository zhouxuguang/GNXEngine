//
//  TerrainCullPass.h
//  GNXEngine
//
//  GPU Compute Shader 地形剔除 Pass
//  使用 Compute Shader 对所有地形 Patch 进行视锥体剔除，
//  输出 Indirect Draw Command 供后续渲染使用。
//

#ifndef GNX_ENGINE_TERRAIN_CULL_PASS_H
#define GNX_ENGINE_TERRAIN_CULL_PASS_H

#include "Runtime/RenderSystem/include/RSDefine.h"
#include "Runtime/RenderSystem/include/FrameGraph/FrameGraph.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"   // CommandBufferPtr, SCOPED_DEBUGMARKER_EVENT
#include "Runtime/RenderCore/include/RCBuffer.h"        // RCBufferPtr, RCBufferDesc

NS_RENDERSYSTEM_BEGIN

/**
 * @brief TerrainCull Pass 的输入参数
 */
struct TerrainCullParams
{
    uint32_t patchCount;              // 总 Patch 数量（来自 QuadTreeTerrain::GetPatchMetaCount()）
    float maxHeight;                 // 地形最大高度（用于构建 AABB Y 轴范围）
};

/**
 * @brief TerrainCull Pass 的输出结果
 */
struct TerrainCullOutput
{
    RenderCore::RCBufferPtr indirectArgsBuffer;   // GPU 写入的 IndirectCommand[] 缓冲区（紧凑排列）
    RenderCore::RCBufferPtr visibleCountBuffer;   // GPU 写入的可见 Patch 计数（1 个 uint）
    uint32_t indirectArgsCount;                  // 缓冲区容量（= patchCount，用于 drawCount 上限）
};

/**
 * @brief GPU Compute Shader 地形剔除 Pass
 *
 * 功能：
 * 1. 接收全部 PatchMeta（未做可见性筛选）作为 SSBO 输入
 * 2. 在 GPU 上执行视锥体剔除（AABB vs 6 平面）
 * 3. 输出 DrawIndexedIndirectCommand 数组（visible=1, culled=0）
 *
 * 流程：
 * CPU: Update() → BuildPatchMetaBuffer() [上传所有叶子 PatchMeta 到 SSBO]
 *       → AddToFrameGraph() [声明资源 + 执行 Compute Shader]
 *       → TerrainComponent 读取 gIndirectArgs 做 DrawIndexedPrimitivesIndirect()
 */
class RENDERSYSTEM_API TerrainCullPass
{
public:
    TerrainCullPass();
    ~TerrainCullPass();

    /**
     * @brief 初始化（创建 Compute Pipeline）
     */
    bool Initialize();

    /**
     * @brief 独立执行 GPU 剔除 Compute Dispatch（不需要 FrameGraph）
     *
     * @param commandBuffer 命令缓冲
     * @param params         剔除参数（patchCount, maxHeight）
     * @param patchMetaSSBO  全部 PatchMeta 的 SSBO
     * @param frameIndex     当前帧索引（用于选择 per-frame 缓冲区）
     * @return 输出：包含 IndirectArgs 缓冲区和数量
     */
    TerrainCullOutput DispatchCull(
        CommandBufferPtr commandBuffer,
        const TerrainCullParams& params,
        RenderCore::RCBufferPtr patchMetaSSBO,
        RenderCore::UniformBufferPtr cameraUBO,
        uint32_t frameIndex);

    /**
     * @brief 添加 TerrainCull Compute Pass 到 FrameGraph（完整集成用）
     *
     * @param passName      Pass 名称
     * @param frameGraph     帧图
     * @param commandBuffer 命令缓冲
     * @param params         剔除参数（patchCount, maxHeight）
     * @param patchMetaSSBO  全部 PatchMeta 的 SSBO
     * @param frameIndex     当前帧索引（用于选择 per-frame 缓冲区）
     * @return 输出：包含 IndirectArgs 缓冲区和数量
     */
    TerrainCullOutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const TerrainCullParams& params,
        RenderCore::RCBufferPtr patchMetaSSBO,
        RenderCore::UniformBufferPtr cameraUBO,
        uint32_t frameIndex);

    /**
     * @brief 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }

private:
    /**
     * @brief 创建 Compute Pipeline（加载 TerrainCull.shader）
     */
    void CreateCullPipeline();

    /**
     * 创建 / 复用指定帧的 Indirect Args 缓冲区
     */
    void EnsureIndirectArgsBuffer(uint32_t count, uint32_t frameIndex);

    /**
     * 创建 / 复用指定帧的 VisibleCount 缓冲区（1 uint，原子计数器）
     */
    void EnsureVisibleCountBuffer(uint32_t frameIndex);

    /**
     * 释放所有 GPU 资源
     */
    void FreeGPUResources();

private:
    static constexpr uint32_t kFrameInFlightCount = 3;

    bool mInitialized = false;

    // Compute Pipeline
    RenderCore::ComputePipelinePtr mCullPipeline = nullptr;

    // Uniform Buffer: cbTerrainCull (patchCount, maxHeight) — 按帧索引三缓冲
    RenderCore::UniformBufferPtr mCullParamsUBOs[kFrameInFlightCount];

    // Indirect Args 输出缓冲区 — 按帧索引三缓冲
    RenderCore::RCBufferPtr mIndirectArgsBuffers[kFrameInFlightCount];
    uint32_t mIndirectArgsCapacities[kFrameInFlightCount] = {};
    RenderCore::RCBufferPtr mRetiredIndirectArgsBuffer;   // resize 时延迟释放

    // VisibleCount 缓冲区 — 按帧索引三缓冲
    RenderCore::RCBufferPtr mVisibleCountBuffers[kFrameInFlightCount];
    RenderCore::RCBufferPtr mRetiredVisibleCountBuffer;   // resize 时延迟释放

    // CommandBuffer 引用（用于创建 Compute Encoder）
    CommandBufferPtr mCommandBuffer;
};

typedef std::shared_ptr<TerrainCullPass> TerrainCullPassPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_TERRAIN_CULL_PASS_H */
