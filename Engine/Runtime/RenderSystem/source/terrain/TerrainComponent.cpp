//
//  TerrainComponent.cpp
//  GNXEngine
//
//  带有 GPU 驱动渲染路径的地形渲染组件。
//  使用模板网格（17x17）+ PatchMeta SSBO + 高度图纹理。
//  VS 从 PatchMeta[instanceID] 读取每个 patch 的世界变换，
//  并采样高度图获取 Y 轴位移。
//

#include "terrain/TerrainComponent.h"
#include "terrain/TerrainCullPass.h"
#include "FrameGraph/FrameGraph.h"              // DispatchCullViaFrameGraph
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// 构造 / 析构
//=============================================================================

TerrainComponent::TerrainComponent() = default;
TerrainComponent::~TerrainComponent() = default;

//=============================================================================
// 初始化
//=============================================================================

void TerrainComponent::InitFromHeightMap(const char* heightmapPath,
                                          float worldSizeXZ,
                                          float heightScale,
                                          uint32_t maxLevel)
{
    mQuadTreeTerrain = QuadTreeTerrain::CreateFromHeightMap(
        heightmapPath, worldSizeXZ, heightScale, maxLevel);

    if (mQuadTreeTerrain)
    {
        mTerrainParamsDirty = true;
        LOG_INFO("TerrainComponent: Initialized from heightmap '%s', maxLevel=%u",
                 heightmapPath, mQuadTreeTerrain->GetMaxLevel());
    }
    else
    {
        LOG_ERROR("TerrainComponent: Failed to load heightmap '%s'", heightmapPath);
    }
}

void TerrainComponent::InitProcedural(uint32_t gridSize,
                                       float worldSizeXZ,
                                       float heightScale,
                                       uint32_t maxLevel)
{
    mQuadTreeTerrain = QuadTreeTerrain::Create(
        gridSize, worldSizeXZ, heightScale, maxLevel);

    if (mQuadTreeTerrain)
    {
        mTerrainParamsDirty = true;
        LOG_INFO("TerrainComponent: Initialized procedural terrain, maxLevel=%u",
                 mQuadTreeTerrain->GetMaxLevel());
    }
}

//=============================================================================
// 材质
//=============================================================================

void TerrainComponent::SetMaterial(const MaterialPtr& material)
{
    mMaterial = material;
}

//=============================================================================
// 每帧更新
//=============================================================================

void TerrainComponent::Update(float deltaTime)
{
    mGPUPathDataPrepared = false;
}

//=============================================================================
// 地形参数 UBO
//=============================================================================

void TerrainComponent::EnsureTerrainUBO()
{
    if (!mQuadTreeTerrain) return;

    cbTerrainParams params;
    params.worldSize     = mQuadTreeTerrain->GetWorldSize();
    params.halfWorldSize = params.worldSize * 0.5f;
    params.uvTileScale   = 1.0f;  // 材质纹理的平铺因子（1.0 = 不平铺）
    params.gridSize      = mQuadTreeTerrain->GetGridSize();

    if (!mTerrainParamsUBO)
    {
        mTerrainParamsUBO = RenderCore::GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbTerrainParams));
        if (mTerrainParamsUBO)
            mTerrainParamsDirty = true;
    }

    if (mTerrainParamsUBO && mTerrainParamsDirty)
    {
        mTerrainParamsUBO->SetData(&params, 0, sizeof(params));
        mTerrainParamsDirty = false;
    }
}

//=============================================================================
// GPU 路径数据准备
//=============================================================================

void TerrainComponent::PrepareGPUPathData(const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || mGPUPathDataPrepared) return;

    // 构建可见 PatchMeta SSBO（含视锥体剔除）
    mQuadTreeTerrain->BuildGPUPathData(frustum);

    // 确保地形参数 UBO 数据最新
    EnsureTerrainUBO();

    mGPUPathDataPrepared = true;
}

//=============================================================================
// GPU 驱动渲染路径（G-Buffer）
//=============================================================================

void TerrainComponent::Render(RenderEncoder* renderEncoder,
                               UniformBufferPtr cameraUBO,
                               UniformBufferPtr objectUBO,
                               GraphicsPipelinePtr terrainGBufferPSO,
                               const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !mMaterial || !renderEncoder)
        return;

    // ========================================================================
    // 路径选择：GPU 剔除（Indirect Draw） vs CPU 实例化绘制
    // ========================================================================
    if (mUseGPUCulling && mCullPass && mLastCullOutput.indirectArgsBuffer)
    {
        // ---- GPU 剔除路径：CS 已分发，使用 Indirect Draw ----
        RenderGPUCulled(renderEncoder, cameraUBO, terrainGBufferPSO);
    }
    else
    {
        // ---- CPU 实例化绘制路径：原始视锥体剔除 + 实例化绘制 ----
        RenderCPUInstanced(renderEncoder, cameraUBO, terrainGBufferPSO, frustum);
    }
}

//=============================================================================
// GPU 剔除渲染路径（Indirect Draw）
//=============================================================================

void TerrainComponent::RenderGPUCulled(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        GraphicsPipelinePtr terrainGBufferPSO)
{
    auto allPatchMeta = mQuadTreeTerrain->GetPatchMetaBuffer();
    uint32_t totalPatchCount = mQuadTreeTerrain->GetPatchMetaCount();
    if (!allPatchMeta || totalPatchCount == 0)
        return;

    auto templateIB = mQuadTreeTerrain->GetTemplateIB();
    auto heightmapTexture = mQuadTreeTerrain->GetHeightMapTexture();
    if (!templateIB || !heightmapTexture)
        return;

    EnsureTerrainUBO();

    // ---- 绑定 PSO 和共享状态 ----
    renderEncoder->SetGraphicsPipeline(terrainGBufferPSO);
    if (mWireframe)
        renderEncoder->SetFillMode(FillModeWireframe);

    // UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // PatchMeta SSBO（全部 patch — GPU 已完成剔除到 indirect args）
    renderEncoder->SetStorageBuffer("gPatchMeta", allPatchMeta,
        RenderCore::ShaderStage_Vertex);

    // 高度图纹理（VS 采样获取 Y 位移）
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // 模板 VB
    renderEncoder->SetVertexBuffer(mQuadTreeTerrain->GetTemplateVB(), 0, 0);

    // 材质纹理
    BindMaterialTextures(renderEncoder);

    // ---- Indirect Draw：GPU 决定每个 patch 的可见性 ----
    renderEncoder->DrawIndexedPrimitivesIndirect(
        PrimitiveMode_TRIANGLES,
        templateIB,
        0,                                    // indexBufferOffset
        mLastCullOutput.indirectArgsBuffer,   // indirect args（GPU 写入）
        0,                                    // indirectBufferOffset
        totalPatchCount,                      // drawCount（= 总 patch 数，被剔除的 instanceCount=0）
        sizeof(RenderCore::DrawIndexedIndirectCommand)  // stride
    );
}

//=============================================================================
// CPU 实例化渲染路径（原始路径）
//=============================================================================

void TerrainComponent::RenderCPUInstanced(RenderEncoder* renderEncoder,
                                           UniformBufferPtr cameraUBO,
                                           GraphicsPipelinePtr terrainGBufferPSO,
                                           const Frustumf* frustum)
{
    // 准备 GPU 路径数据（CPU 视锥体剔除 + 可见 PatchMeta SSBO）
    PrepareGPUPathData(frustum);

    uint32_t visibleCount = mQuadTreeTerrain->GetVisiblePatchMetaCount();
    if (visibleCount == 0)
        return;

    auto templateVB = mQuadTreeTerrain->GetTemplateVB();
    auto templateIB = mQuadTreeTerrain->GetTemplateIB();
    auto visiblePatchMeta = mQuadTreeTerrain->GetVisiblePatchMetaBuffer();
    auto heightmapTexture = mQuadTreeTerrain->GetHeightMapTexture();

    if (!templateVB || !templateIB || !visiblePatchMeta || !heightmapTexture)
        return;

    // ---- 绑定 PSO 和共享状态 ----
    renderEncoder->SetGraphicsPipeline(terrainGBufferPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    // 绑定 UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // 绑定 PatchMeta SSBO 到顶点阶段（VS 读取 gPatchMeta[instanceID]）
    renderEncoder->SetStorageBuffer("gPatchMeta", visiblePatchMeta,
        RenderCore::ShaderStage_Vertex);

    // 绑定高度图纹理 + 采样器
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // 绑定模板网格顶点缓冲区
    renderEncoder->SetVertexBuffer(templateVB, 0, 0);

    // 材质纹理
    BindMaterialTextures(renderEncoder);

    // ---- 实例化绘制：每个可见 patch 一个实例 ----
    renderEncoder->DrawIndexedInstancePrimitives(
        PrimitiveMode_TRIANGLES,
        384,          // 9x9 模板网格的索引数（8*8*6）
        templateIB,
        0,             // indexBufferOffset
        0,             // firstInstance
        visibleCount); // instanceCount = 可见 patch 数量
}

//=============================================================================
// GPU 驱动渲染路径（仅深度）
//=============================================================================

void TerrainComponent::RenderDepthOnly(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        UniformBufferPtr objectUBO,
                                        GraphicsPipelinePtr terrainDepthPSO,
                                        const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !renderEncoder || !terrainDepthPSO)
        return;

    // 路径选择
    if (mUseGPUCulling && mCullPass && mLastCullOutput.indirectArgsBuffer)
    {
        // GPU 剔除路径：Indirect Draw
        RenderDepthGPUCulled(renderEncoder, cameraUBO, terrainDepthPSO);
    }
    else
    {
        // CPU 实例化绘制路径
        RenderDepthCPUInstanced(renderEncoder, cameraUBO, terrainDepthPSO, frustum);
    }
}

//=============================================================================
// 仅深度 GPU 剔除路径（Indirect Draw）
//=============================================================================

void TerrainComponent::RenderDepthGPUCulled(RenderEncoder* renderEncoder,
                                             UniformBufferPtr cameraUBO,
                                             GraphicsPipelinePtr terrainDepthPSO)
{
    auto allPatchMeta = mQuadTreeTerrain->GetPatchMetaBuffer();
    uint32_t totalPatchCount = mQuadTreeTerrain->GetPatchMetaCount();
    if (!allPatchMeta || totalPatchCount == 0)
        return;

    auto templateIB = mQuadTreeTerrain->GetTemplateIB();
    auto heightmapTexture = mQuadTreeTerrain->GetHeightMapTexture();
    if (!templateIB || !heightmapTexture)
        return;

    EnsureTerrainUBO();

    renderEncoder->SetGraphicsPipeline(terrainDepthPSO);
    if (mWireframe)
        renderEncoder->SetFillMode(FillModeWireframe);

    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    renderEncoder->SetStorageBuffer("gPatchMeta", allPatchMeta,
        RenderCore::ShaderStage_Vertex);

    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    renderEncoder->SetVertexBuffer(mQuadTreeTerrain->GetTemplateVB(), 0, 0);

    // Indirect Draw
    renderEncoder->DrawIndexedPrimitivesIndirect(
        PrimitiveMode_TRIANGLES,
        templateIB,
        0,
        mLastCullOutput.indirectArgsBuffer,
        0,
        totalPatchCount,
        sizeof(RenderCore::DrawIndexedIndirectCommand)
    );
}

//=============================================================================
// 仅深度 CPU 实例化路径（原始路径）
//=============================================================================

void TerrainComponent::RenderDepthCPUInstanced(RenderEncoder* renderEncoder,
                                                UniformBufferPtr cameraUBO,
                                                GraphicsPipelinePtr terrainDepthPSO,
                                                const Frustumf* frustum)
{
    PrepareGPUPathData(frustum);

    uint32_t visibleCount = mQuadTreeTerrain->GetVisiblePatchMetaCount();
    if (visibleCount == 0)
        return;

    auto templateVB = mQuadTreeTerrain->GetTemplateVB();
    auto templateIB = mQuadTreeTerrain->GetTemplateIB();
    auto visiblePatchMeta = mQuadTreeTerrain->GetVisiblePatchMetaBuffer();
    auto heightmapTexture = mQuadTreeTerrain->GetHeightMapTexture();

    if (!templateVB || !templateIB || !visiblePatchMeta || !heightmapTexture)
        return;

    renderEncoder->SetGraphicsPipeline(terrainDepthPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    renderEncoder->SetStorageBuffer("gPatchMeta", visiblePatchMeta,
        RenderCore::ShaderStage_Vertex);

    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    renderEncoder->SetVertexBuffer(templateVB, 0, 0);

    renderEncoder->DrawIndexedInstancePrimitives(
        PrimitiveMode_TRIANGLES,
        384,
        templateIB,
        0,
        0,
        visibleCount);
}

//=============================================================================
// 访问器
//=============================================================================

float TerrainComponent::GetHeight(float worldX, float worldZ) const
{
    if (!mQuadTreeTerrain)
        return 0.0f;
    return mQuadTreeTerrain->GetHeight(worldX, worldZ);
}

void TerrainComponent::SetLODDistanceFactor(float factor)
{
    if (mQuadTreeTerrain)
        mQuadTreeTerrain->SetLODDistanceFactor(factor);
}

void TerrainComponent::SetSSEThreshold(float threshold)
{
    if (mQuadTreeTerrain)
        mQuadTreeTerrain->SetSSEThreshold(threshold);
}

void TerrainComponent::SetWireframe(bool wireframe)
{
    mWireframe = wireframe;
}

//=============================================================================
// GPU 剔除开关
//=============================================================================

void TerrainComponent::SetUseGPUCulling(bool enable)
{
    mUseGPUCulling = enable;

    if (enable && !mCullPass)
    {
        mCullPass = std::make_shared<TerrainCullPass>();
        if (!mCullPass->Initialize())
        {
            LOG_ERROR("TerrainComponent: TerrainCullPass 初始化失败，回退到 CPU 剔除");
            mCullPass.reset();
            mUseGPUCulling = false;
        }
    }

    LOG_INFO("TerrainComponent: GPU 剔除 %s", enable ? "已启用" : "已禁用（使用 CPU 实例化绘制）");
}

//=============================================================================
// DispatchGPUCull - 执行 GPU Compute Shader 剔除（在 Render 之前调用）
//=============================================================================

void TerrainComponent::DispatchGPUCull(CommandBufferPtr commandBuffer,
                                        const mathutil::Matrix4x4f& vpMatrix,
                                        RenderCore::UniformBufferPtr cameraUBO)
{
    if (!mUseGPUCulling || !mCullPass || !mCullPass->IsInitialized() || !mQuadTreeTerrain)
    {
        // 清除缓存输出，使 Render() 回退到 CPU 路径
        mLastCullOutput.indirectArgsBuffer = nullptr;
        mLastCullOutput.indirectArgsCount = 0;
        return;
    }

    // 构建全部 patch 元数据缓冲区（不仅是可见的 — GPU 负责剔除）
    // 注意：BuildPatchMetaBuffer() 在 BuildGPUPathData 内部调用，或我们直接调用
    // 对于 GPU 路径，我们需要从 GetPatchMetaBuffer() 获取完整缓冲区
    auto allPatchMeta = mQuadTreeTerrain->GetPatchMetaBuffer();
    uint32_t totalPatchCount = mQuadTreeTerrain->GetPatchMetaCount();

    if (!allPatchMeta || totalPatchCount == 0)
    {
        mLastCullOutput.indirectArgsBuffer = nullptr;
        mLastCullOutput.indirectArgsCount = 0;
        return;
    }

    EnsureTerrainUBO();

    TerrainCullParams params;
    params.patchCount = totalPatchCount;
    params.maxHeight = mQuadTreeTerrain->GetHeightScale();

    // 分发 compute shader 剔除
    mLastCullOutput = mCullPass->DispatchCull(commandBuffer, params, allPatchMeta, cameraUBO);

    LOG_DEBUG("TerrainComponent::DispatchGPUCull: %u 个 patch，indirect args 缓冲区 %s",
              totalPatchCount,
              mLastCullOutput.indirectArgsBuffer ? "ready" : "FAILED");
}

//=============================================================================
// DispatchCullViaFrameGraph - FrameGraph 集成的 GPU 剔除（推荐方式）
//=============================================================================

void TerrainComponent::DispatchCullViaFrameGraph(FrameGraph& frameGraph,
                                                   CommandBufferPtr commandBuffer,
                                                   const mathutil::Matrix4x4f& vpMatrix,
                                                   RenderCore::UniformBufferPtr cameraUBO)
{
    if (!mUseGPUCulling || !mCullPass || !mCullPass->IsInitialized() || !mQuadTreeTerrain)
    {
        mLastCullOutput.indirectArgsBuffer = nullptr;
        mLastCullOutput.indirectArgsCount = 0;
        return;
    }

    auto allPatchMeta = mQuadTreeTerrain->GetPatchMetaBuffer();
    uint32_t totalPatchCount = mQuadTreeTerrain->GetPatchMetaCount();

    if (!allPatchMeta || totalPatchCount == 0)
    {
        mLastCullOutput.indirectArgsBuffer = nullptr;
        mLastCullOutput.indirectArgsCount = 0;
        return;
    }

    EnsureTerrainUBO();

    TerrainCullParams params;
    params.patchCount = totalPatchCount;
    params.maxHeight = mQuadTreeTerrain->GetHeightScale();

    // 注册到 FrameGraph 作为 Compute Pass
    mLastCullOutput = mCullPass->AddToFrameGraph(
        "TerrainCull", frameGraph, commandBuffer, params, allPatchMeta, cameraUBO);

    LOG_DEBUG("TerrainComponent::DispatchCullViaFrameGraph: %u 个 patch 已注册到 FrameGraph",
              totalPatchCount);
}

//=============================================================================
// BindMaterialTextures - 共享材质纹理绑定（DRY 辅助函数）
//=============================================================================

void TerrainComponent::BindMaterialTextures(RenderEncoder* renderEncoder)
{
    TextureSamplerPtr textureSampler;
    if (auto mesh = mQuadTreeTerrain->GetMesh())
        textureSampler = mesh->GetSampler();
    if (!textureSampler)
    {
        RenderCore::SamplerDesc defaultDesc;
        defaultDesc.filterMag = RenderCore::MAG_LINEAR;
        defaultDesc.filterMin = RenderCore::MIN_LINEAR;
        defaultDesc.filterMip = RenderCore::MIN_LINEAR_MIPMAP_LINEAR;
        defaultDesc.anisotropyLog2 = 2;
        defaultDesc.wrapS = RenderCore::REPEAT;
        defaultDesc.wrapT = RenderCore::REPEAT;
        textureSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(defaultDesc);
    }

    auto diffuseSlot = mMaterial->GetTextureSlot("diffuseTexture");
    if (diffuseSlot)
    {
        RenderCore::SamplerDesc optimizedDesc = diffuseSlot->samplerDesc;
        optimizedDesc.filterMin = RenderCore::MIN_LINEAR;
        optimizedDesc.filterMip = RenderCore::MIN_LINEAR_MIPMAP_LINEAR;
        optimizedDesc.anisotropyLog2 = 2;
        optimizedDesc.wrapS = RenderCore::REPEAT;
        optimizedDesc.wrapT = RenderCore::REPEAT;

        auto sampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(optimizedDesc);
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", diffuseSlot->texture, sampler);
    }
    else
    {
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", nullptr, textureSampler);
    }
}

NS_RENDERSYSTEM_END
