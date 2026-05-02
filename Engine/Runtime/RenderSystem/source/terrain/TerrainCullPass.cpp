//
//  TerrainCullPass.cpp
//  GNXEngine
//
//  GPU Compute Shader 地形剔除 Pass 实现
//

#include "terrain/TerrainCullPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/ComputeEncoder.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"   // SCOPED_DEBUGMARKER_EVENT, CommandBufferPtr
#include "Runtime/RenderCore/include/RCBuffer.h"        // RCBufferDesc
#include "Runtime/BaseLib/include/LogService.h"         // LOG_ERROR, LOG_INFO, LOG_WARN, LOG_DEBUG
#include "Runtime/MathUtil/include/Matrix4x4.h"

USING_NS_RENDERCORE
USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// cbTerrainCull: 匹配着色器中 cbuffer cbTerrainCull 的布局
//=============================================================================

struct cbTerrainCull
{
    uint32_t patchCount;    // gPatchMeta 数组长度
    float     maxHeight;     // 地形最大高度（AABB Y 轴上界）
    uint32_t pad0;
    uint32_t pad1;

    // VP 矩阵（4x4 = 16 floats，紧跟在上述字段之后）
    // 着色器中通过 cbTerrainCull.gPatchCount 访问前4个字段
    // 通过 MATRIX_VP 访问视投影矩阵
    // 注意：UBO 按 vec4 对齐，maxHeight 后面有 2 个 uint padding 使其到下一个 vec4 边界
    // 然后 VP 矩阵 (16 bytes) 自然对齐
};

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

    // 创建参数 UBO
    mCullParamsUBO = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbTerrainCull));
    if (!mCullParamsUBO)
    {
        LOG_ERROR("TerrainCullPass: Failed to create params UBO");
        return false;
    }

    mInitialized = true;
    LOG_INFO("TerrainCullPass: Initialized successfully");
    return true;
}

//=============================================================================
// CreateCullPipeline - 加载 TerrainCull.shader 并创建 Compute Pipeline
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
// EnsureIndirectArgsBuffer - 确保 IndirectArgs 缓冲区足够大
//=============================================================================

void TerrainCullPass::EnsureIndirectArgsBuffer(uint32_t count)
{
    if (count == 0) return;

    // 缓冲区已够大，复用
    if (mIndirectArgsBuffer && mIndirectArgsCapacity >= count)
        return;

    auto renderDevice = GetRenderDevice();

    // 每个 DrawIndexedIndirectCommand = 5 * uint32 = 20 bytes
    uint32_t dataSize = count * sizeof(RenderCore::DrawIndexedIndirectCommand);

    RenderCore::RCBufferDesc desc(dataSize,
        RenderCore::RCBufferUsage::IndirectBuffer,   // Indirect 用途
        RenderCore::StorageModeShared);               // GPU 写入，CPU 读取结果

    mIndirectArgsBuffer = renderDevice->CreateBuffer(desc);
    mIndirectArgsCapacity = count;

    if (mIndirectArgsBuffer)
        mIndirectArgsBuffer->SetName("TerrainCull_IndirectArgs");

    LOG_INFO("TerrainCullPass: IndirectArgs buffer created, capacity=%u (%u bytes)",
             count, dataSize);
}

//=============================================================================
// DispatchCull - 独立执行 GPU 剔除（不需要 FrameGraph)
//=============================================================================

TerrainCullOutput TerrainCullPass::DispatchCull(
    CommandBufferPtr commandBuffer,
    const TerrainCullParams& params,
    RenderCore::RCBufferPtr patchMetaSSBO,
    RenderCore::UniformBufferPtr cameraUBO)
{
    // 确保 IndirectArgs 缓冲区足够大
    EnsureIndirectArgsBuffer(params.patchCount);

    if (!mCullPipeline || !mCullParamsUBO || !mIndirectArgsBuffer || !patchMetaSSBO || !commandBuffer)
    {
        LOG_WARN("TerrainCullPass::DispatchCull: Missing required resources, skipping");
        TerrainCullOutput output;
        output.indirectArgsBuffer = nullptr;
        output.indirectArgsCount = 0;
        return output;
    }

    // ---- 创建 Compute Encoder ----
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    if (!computeEncoder)
    {
        LOG_ERROR("TerrainCullPass::DispatchCull: Failed to create compute encoder");
        TerrainCullOutput output;
        output.indirectArgsBuffer = nullptr;
        output.indirectArgsCount = 0;
        return output;
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
    mCullParamsUBO->SetData(&cullParams, 0, sizeof(cullParams));
    computeEncoder->SetUniformBuffer("cbTerrainCull", mCullParamsUBO);

    // ---- 绑定 PatchMeta SSBO（输入：所有待剔除的 Patch）----
    computeEncoder->SetStorageBuffer("gPatchMeta", patchMetaSSBO);

    // ---- 绑定 IndirectArgs RW Buffer（输出：剔除结果）----
    computeEncoder->SetStorageBuffer("gIndirectArgs", mIndirectArgsBuffer);

    // ---- Dispatch：每个线程组处理 64 个 Patch ----
    uint32_t threadGroupsX = (params.patchCount + 63) / 64;
    computeEncoder->Dispatch(threadGroupsX, 1, 1);

    // ---- 结束编码 ----
    computeEncoder->EndEncode();

    // 返回输出
    TerrainCullOutput output;
    output.indirectArgsBuffer = mIndirectArgsBuffer;
    output.indirectArgsCount   = params.patchCount;

    LOG_DEBUG("TerrainCullPass::DispatchCull: Dispatched %u thread groups for %u patches",
              threadGroupsX, params.patchCount);

    return output;
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
    RenderCore::UniformBufferPtr cameraUBO)
{
    mCommandBuffer = commandBuffer;

    // 确保 IndirectArgs 缓冲区足够大
    EnsureIndirectArgsBuffer(params.patchCount);

    if (!mCullPipeline || !mCullParamsUBO || !mIndirectArgsBuffer || !patchMetaSSBO)
    {
        LOG_WARN("TerrainCullPass: Missing required resources, skipping cull pass");
        TerrainCullOutput output;
        output.indirectArgsBuffer = nullptr;
        output.indirectArgsCount = 0;
        return output;
    }

    // 定义 Pass 数据结构
    struct CullPassData
    {
        // 无 FrameGraph 资源声明 — 我们使用外部导入的 buffer
        // （PatchMeta SSBO 和 IndirectArgs Buffer 由调用方管理生命周期）
    };

    // 添加 Compute Pass 到 FrameGraph
    const auto& data = frameGraph.AddPass<CullPassData>(
        passName,
        // Setup 阶段：声明这是一个 SideEffect Pass（读写外部资源）
        [&](FrameGraph::Builder& builder, CullPassData& passData)
        {
            // 标记为 SideEffect：此 Pass 不产生/消费 FrameGraph 资源，
            // 但会修改外部导入的 IndirectArgs Buffer
            builder.SetSideEffect();
            builder.EnableAsyncCompute(false);
        },
        // Execute 阶段：执行 Compute Shader
        [&, patchMetaSSBO, cameraUBO](const CullPassData& /*passData*/, FrameGraphPassResources& resources, void* /*context*/)
        {
            float debugColor[4] = {0.2f, 0.8f, 0.2f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(mCommandBuffer, resources.GetPassName().c_str(), debugColor);

            // ---- 创建 Compute Encoder ----
            ComputeEncoderPtr computeEncoder = mCommandBuffer->CreateComputeEncoder();
            if (!computeEncoder)
            {
                LOG_ERROR("TerrainCullPass: Failed to create compute encoder");
                return;
            }

            // ---- 绑定 Compute Pipeline ----
            computeEncoder->SetComputePipeline(mCullPipeline);

            // ---- 设置参数 UBO（patchCount + maxHeight + VP矩阵）----
            cbTerrainCull cullParams;
            memset(&cullParams, 0, sizeof(cullParams));
            cullParams.patchCount  = params.patchCount;
            cullParams.maxHeight  = params.maxHeight;
            // VP 矩阵紧随在后面（从 offset=16 开始）
            // 注意：MATRIX_VP 在 GNXEngineVariables.hlsl 中定义，由引擎自动设置
            mCullParamsUBO->SetData(&cullParams, 0, sizeof(cullParams));
            computeEncoder->SetUniformBuffer("cbTerrainCull", mCullParamsUBO);

            // ---- 绑定相机参数（cbPerCamera 提供 MATRIX_VP 给 shader）----
            if (cameraUBO)
                computeEncoder->SetUniformBuffer("cbPerCamera", cameraUBO);

            // ---- 绑定 PatchMeta SSBO（输入：所有待剔除的 Patch）----
            computeEncoder->SetStorageBuffer("gPatchMeta", patchMetaSSBO);

            // ---- 绑定 IndirectArgs RW Buffer（输出：剔除结果）----
            computeEncoder->SetStorageBuffer("gIndirectArgs", mIndirectArgsBuffer);

            // ---- Dispatch：每个线程组处理一个 Patch ----
            // [numthreads(64,1,1)] — 与 TerrainCull.shader 中一致
            uint32_t threadGroupsX = (params.patchCount + 63) / 64;  // 向上取整
            computeEncoder->Dispatch(threadGroupsX, 1, 1);

            // ---- 结束编码 ----
            computeEncoder->EndEncode();
        }
    );

    // 返回输出
    TerrainCullOutput output;
    output.indirectArgsBuffer = mIndirectArgsBuffer;
    output.indirectArgsCount = params.patchCount;  // 缓冲区大小 = patchCount（部分 instanceCount=0）

    return output;
}

//=============================================================================
// FreeGPUResources
//=============================================================================

void TerrainCullPass::FreeGPUResources()
{
    mCullPipeline.reset();
    mCullParamsUBO.reset();
    mIndirectArgsBuffer.reset();
    mIndirectArgsCapacity = 0;
    mInitialized = false;
}

NS_RENDERSYSTEM_END
