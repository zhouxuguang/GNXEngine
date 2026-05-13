//
//  TerrainCullPass.cpp
//  GNXEngine
//
//  GPU Compute Shader 地形剔除 Pass 实现
//  使用原子紧凑化：可见 patch 从 index 0 开始连续排列，
//  visibleCount 记录可见数量，indirectArgs 其余条目 instanceCount=0。
//

#include "terrain/TerrainCullPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/ComputeEncoder.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"   // SCOPED_DEBUGMARKER_EVENT, CommandBufferPtr
#include "Runtime/RenderCore/include/RCBuffer.h"        // RCBufferDesc
#include "Runtime/BaseLib/include/LogService.h"         // LOG_ERROR, LOG_INFO, LOG_WARN, LOG_DEBUG

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// cbTerrainCull: 匹配着色器中 cbuffer cbTerrainCull 的布局（TerrainCommon.hlsl）
//=============================================================================

struct cbTerrainCull
{
    uint32_t patchCount;    // gPatchMeta 数组长度
    float    maxHeight;     // 地形最大高度（AABB Y 轴上界）
    uint32_t pad0;
    uint32_t pad1;
};

//=============================================================================
// Helper: CPU 端清零缓冲区
//=============================================================================

static void ZeroFillBuffer(const RenderCore::RCBufferPtr& buffer, uint32_t size)
{
    if (!buffer || size == 0) return;
    void* mapped = buffer->Map();
    if (mapped)
    {
        memset(mapped, 0, size);
        buffer->Unmap();
    }
}

//=============================================================================
// Construction / Destruction
//=============================================================================

TerrainCullPass::TerrainCullPass()
{
}

TerrainCullPass::~TerrainCullPass()
{
    FreeGPUResources();
}

//=============================================================================
// Initialize
//=============================================================================

bool TerrainCullPass::Initialize()
{
    if (mInitialized)
        return true;

    CreateCullPipeline();

    // 创建参数 UBO（每帧一个，避免跨帧 SetData 竞态）
    for (uint32_t i = 0; i < kFrameInFlightCount; ++i)
    {
        mCullParamsUBOs[i] = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbTerrainCull));
        if (!mCullParamsUBOs[i])
        {
            LOG_ERROR("TerrainCullPass: Failed to create params UBO for frame %u", i);
            return false;
        }
    }

    mInitialized = true;
    LOG_INFO("TerrainCullPass: Initialized successfully");
    return true;
}

//=============================================================================
// CreateCullPipeline
//=============================================================================

void TerrainCullPass::CreateCullPipeline()
{
    ShaderAssetString shaderAsset = LoadShaderAsset("TerrainCull");

    mCullPipeline = GetRenderDevice()->CreateComputePipeline(*shaderAsset.computeShader->shaderSource);

    if (!mCullPipeline)
    {
        LOG_ERROR("TerrainCullPass: Failed to create compute pipeline from TerrainCull.shader");
        return;
    }

    LOG_INFO("TerrainCullPass: Compute pipeline created from TerrainCull.shader");
}

//=============================================================================
// EnsureIndirectArgsBuffer - 确保指定帧的 IndirectArgs 缓冲区足够大
//=============================================================================

void TerrainCullPass::EnsureIndirectArgsBuffer(uint32_t count, uint32_t frameIndex)
{
    if (count == 0) return;

    auto& buffer = mIndirectArgsBuffers[frameIndex];
    auto& capacity = mIndirectArgsCapacities[frameIndex];

    // 缓冲区已够大，复用
    if (buffer && capacity >= count)
        return;

    auto renderDevice = GetRenderDevice();

    uint32_t dataSize = count * sizeof(RenderCore::DrawIndexedIndirectCommand);

    // SSBO（Compute 写入）+ IndirectBuffer（DrawIndexedPrimitivesIndirect 读取）
    RenderCore::RCBufferDesc desc(dataSize,
        RenderCore::RCBufferUsage::StorageBuffer | RenderCore::RCBufferUsage::IndirectBuffer,
        RenderCore::StorageModeShared);

    // 将当前缓冲区移入退役列表，延迟释放（防止 GPU 仍在使用）
    mRetiredIndirectArgsBuffer = buffer;

    buffer = renderDevice->CreateBuffer(desc);
    capacity = count;

    if (buffer)
    {
        char name[64];
        snprintf(name, sizeof(name), "TerrainCull_IndirectArgs_F%u", frameIndex);
        buffer->SetName(name);
    }

    LOG_INFO("TerrainCullPass: IndirectArgs buffer created for frame %u, capacity=%u (%u bytes)",
             frameIndex, count, dataSize);
}

//=============================================================================
// EnsureVisibleCountBuffer - 创建/复用指定帧的原子计数器缓冲区（1 uint）
//=============================================================================

void TerrainCullPass::EnsureVisibleCountBuffer(uint32_t frameIndex)
{
    auto& buffer = mVisibleCountBuffers[frameIndex];

    if (buffer)
        return;

    auto renderDevice = GetRenderDevice();

    // 1 个 uint = 4 bytes，使用 SSBO 供 Compute Shader 原子递增
    // 同时需要 IndirectBuffer 用途，因为 DrawIndexedPrimitivesIndirectCount 将其作为 countBuffer 读取
    RenderCore::RCBufferDesc desc(4,
        RenderCore::RCBufferUsage::StorageBuffer | RenderCore::RCBufferUsage::IndirectBuffer,
        RenderCore::StorageModeShared);

    mRetiredVisibleCountBuffer = buffer;
    buffer = renderDevice->CreateBuffer(desc);

    if (buffer)
    {
        char name[64];
        snprintf(name, sizeof(name), "TerrainCull_VisibleCount_F%u", frameIndex);
        buffer->SetName(name);
    }
}

//=============================================================================
// DispatchCull - 独立执行 GPU 剔除（不需要 FrameGraph)
//=============================================================================

TerrainCullOutput TerrainCullPass::DispatchCull(
    CommandBufferPtr commandBuffer,
    const TerrainCullParams& params,
    RenderCore::RCBufferPtr patchMetaSSBO,
    RenderCore::UniformBufferPtr cameraUBO,
    uint32_t frameIndex)
{
    uint32_t fi = frameIndex % kFrameInFlightCount;

    // 确保缓冲区就绪
    EnsureIndirectArgsBuffer(params.patchCount, fi);
    EnsureVisibleCountBuffer(fi);

    auto& indirectArgs = mIndirectArgsBuffers[fi];
    auto& visibleCount = mVisibleCountBuffers[fi];
    auto& cullParamsUBO = mCullParamsUBOs[fi];

    if (!mCullPipeline || !cullParamsUBO || !indirectArgs || !visibleCount || !patchMetaSSBO || !commandBuffer)
    {
        LOG_WARN("TerrainCullPass::DispatchCull: Missing required resources, skipping");
        return { nullptr, nullptr, 0 };
    }

    // ---- CPU 端清零（per-frame buffer 安全：该帧 slot 已被 fence 保护） ----
    ZeroFillBuffer(visibleCount, 4);
    ZeroFillBuffer(indirectArgs, params.patchCount * sizeof(RenderCore::DrawIndexedIndirectCommand));

    // ---- 创建 Compute Encoder ----
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    if (!computeEncoder)
    {
        LOG_ERROR("TerrainCullPass::DispatchCull: Failed to create compute encoder");
        return { nullptr, nullptr, 0 };
    }

    float debugColor[4] = {0.2f, 0.8f, 0.2f, 1.0f};
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, "TerrainCull_Dispatch", debugColor);

    // ---- 绑定 Compute Pipeline ----
    computeEncoder->SetComputePipeline(mCullPipeline);

    // ---- 设置参数 UBO（patchCount + maxHeight）----
    cbTerrainCull cullParams;
    memset(&cullParams, 0, sizeof(cullParams));
    cullParams.patchCount = params.patchCount;
    cullParams.maxHeight  = params.maxHeight;
    cullParamsUBO->SetData(&cullParams, 0, sizeof(cullParams));
    computeEncoder->SetUniformBuffer("cbTerrainCull", cullParamsUBO);

    // ---- 绑定相机参数（cbPerCamera 提供 frustumPlanes 给 shader）----
    if (cameraUBO)
        computeEncoder->SetUniformBuffer("cbPerCamera", cameraUBO);

    // ---- 绑定 PatchMeta SSBO（输入）----
    computeEncoder->SetStorageBuffer("gPatchMeta", patchMetaSSBO);

    // ---- 绑定 IndirectArgs RW Buffer（输出：紧凑化剔除结果）----
    computeEncoder->SetStorageBuffer("gIndirectArgs", indirectArgs);

    // ---- 绑定 VisibleCount RW Buffer（输出：可见 Patch 计数）----
    computeEncoder->SetStorageBuffer("visibleCount", visibleCount);

    // ---- Dispatch：[numthreads(128,1,1)] ----
    uint32_t threadGroupsX = (params.patchCount + 127) / 128;
    computeEncoder->Dispatch(threadGroupsX, 1, 1);

    // ---- 结束编码 ----
    computeEncoder->EndEncode();

    // ---- 插入管线屏障：Compute Shader 写入 → Indirect Draw 读取 ----
    commandBuffer->ResourceBarrier(indirectArgs, RenderCore::ResourceAccessType::IndirectCommandRead);
    commandBuffer->ResourceBarrier(visibleCount, RenderCore::ResourceAccessType::IndirectCommandRead);

    LOG_DEBUG("TerrainCullPass::DispatchCull: Dispatched %u thread groups for %u patches (frame %u)",
              threadGroupsX, params.patchCount, fi);

    return { indirectArgs, visibleCount, params.patchCount };
}

//=============================================================================
// AddToFrameGraph - 添加 Compute Shader 剔除 Pass 到 FrameGraph
//=============================================================================

TerrainCullOutput TerrainCullPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const TerrainCullParams& params,
    RenderCore::RCBufferPtr patchMetaSSBO,
    RenderCore::UniformBufferPtr cameraUBO,
    uint32_t frameIndex)
{
    mCommandBuffer = commandBuffer;

    uint32_t fi = frameIndex % kFrameInFlightCount;

    // 确保缓冲区就绪
    EnsureIndirectArgsBuffer(params.patchCount, fi);
    EnsureVisibleCountBuffer(fi);

    auto& indirectArgs = mIndirectArgsBuffers[fi];
    auto& visibleCount = mVisibleCountBuffers[fi];
    auto& cullParamsUBO = mCullParamsUBOs[fi];

    if (!mCullPipeline || !cullParamsUBO || !indirectArgs || !visibleCount || !patchMetaSSBO)
    {
        LOG_WARN("TerrainCullPass: Missing required resources, skipping cull pass");
        return { nullptr, nullptr, 0 };
    }

    // ---- CPU 端清零（per-frame buffer 安全） ----
    ZeroFillBuffer(visibleCount, 4);
    ZeroFillBuffer(indirectArgs, params.patchCount * sizeof(RenderCore::DrawIndexedIndirectCommand));

    // 定义 Pass 数据结构
    struct CullPassData
    {
    };

    // 添加 Compute Pass 到 FrameGraph
    frameGraph.AddPass<CullPassData>(
        passName,
        [&](FrameGraph::Builder& builder, CullPassData& passData)
        {
            builder.SetSideEffect();
            builder.EnableAsyncCompute(false);
        },
        [this, fi, params, patchMetaSSBO, cameraUBO](const CullPassData&, FrameGraphPassResources& resources, void*)
        {
            auto& indirectArgsBuf = mIndirectArgsBuffers[fi];
            auto& visibleCountBuf = mVisibleCountBuffers[fi];
            auto& cullParamsUBO = mCullParamsUBOs[fi];

            float debugColor[4] = {0.2f, 0.8f, 0.2f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(mCommandBuffer, resources.GetPassName().c_str(), debugColor);

            ComputeEncoderPtr computeEncoder = mCommandBuffer->CreateComputeEncoder();
            if (!computeEncoder)
            {
                LOG_ERROR("TerrainCullPass: Failed to create compute encoder");
                return;
            }

            // ---- 绑定 Compute Pipeline ----
            computeEncoder->SetComputePipeline(mCullPipeline);

            // ---- 设置参数 UBO ----
            cbTerrainCull cullParams;
            memset(&cullParams, 0, sizeof(cullParams));
            cullParams.patchCount = params.patchCount;
            cullParams.maxHeight  = params.maxHeight;
            cullParamsUBO->SetData(&cullParams, 0, sizeof(cullParams));
            computeEncoder->SetUniformBuffer("cbTerrainCull", cullParamsUBO);

            // ---- 绑定相机参数（cbPerCamera 提供 frustumPlanes）----
            if (cameraUBO)
                computeEncoder->SetUniformBuffer("cbPerCamera", cameraUBO);

            // ---- 绑定 SSBOs ----
            computeEncoder->SetStorageBuffer("gPatchMeta", patchMetaSSBO);
            computeEncoder->SetStorageBuffer("gIndirectArgs", indirectArgsBuf);
            computeEncoder->SetStorageBuffer("visibleCount", visibleCountBuf);

            // ---- Dispatch ----
            uint32_t threadGroupsX = (params.patchCount + 127) / 128;
            computeEncoder->Dispatch(threadGroupsX, 1, 1);

            computeEncoder->EndEncode();

            // 管线屏障：Compute Shader 写入 → Indirect Draw 读取
            mCommandBuffer->ResourceBarrier(indirectArgsBuf, RenderCore::ResourceAccessType::IndirectCommandRead);
            mCommandBuffer->ResourceBarrier(visibleCountBuf, RenderCore::ResourceAccessType::IndirectCommandRead);
        }
    );

    return { indirectArgs, visibleCount, params.patchCount };
}

//=============================================================================
// FreeGPUResources
//=============================================================================

void TerrainCullPass::FreeGPUResources()
{
    mCullPipeline.reset();
    for (uint32_t i = 0; i < kFrameInFlightCount; ++i)
    {
        mCullParamsUBOs[i].reset();
        mIndirectArgsBuffers[i].reset();
        mVisibleCountBuffers[i].reset();
        mIndirectArgsCapacities[i] = 0;
    }
    mRetiredIndirectArgsBuffer.reset();
    mRetiredVisibleCountBuffer.reset();
    mInitialized = false;
}

NS_RENDERSYSTEM_END
