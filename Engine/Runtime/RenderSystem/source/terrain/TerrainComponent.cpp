//
//  TerrainComponent.cpp
//  GNXEngine
//
//  Terrain rendering component with GPU-driven rendering path.
//  Uses template mesh (17x17) + PatchMeta SSBO + heightmap texture.
//  VS reads PatchMeta[instanceID] for per-patch world transform and
//  samples heightmap for Y displacement.
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
// Construction / Destruction
//=============================================================================

TerrainComponent::TerrainComponent() = default;
TerrainComponent::~TerrainComponent() = default;

//=============================================================================
// Initialization
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
// Material
//=============================================================================

void TerrainComponent::SetMaterial(const MaterialPtr& material)
{
    mMaterial = material;
}

//=============================================================================
// Per-frame update
//=============================================================================

void TerrainComponent::Update(float deltaTime)
{
    mGPUPathDataPrepared = false;
}

//=============================================================================
// Terrain params UBO
//=============================================================================

void TerrainComponent::EnsureTerrainUBO()
{
    if (!mQuadTreeTerrain) return;

    cbTerrainParams params;
    params.worldSize     = mQuadTreeTerrain->GetWorldSize();
    params.halfWorldSize = params.worldSize * 0.5f;
    params.uvTileScale   = 1.0f;  // tiling factor for material textures (1.0 = no tiling)
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
// GPU path data preparation
//=============================================================================

void TerrainComponent::PrepareGPUPathData(const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || mGPUPathDataPrepared) return;

    // Build visible PatchMeta SSBO (with frustum culling)
    mQuadTreeTerrain->BuildGPUPathData(frustum);

    // Ensure terrain params UBO is up to date
    EnsureTerrainUBO();

    mGPUPathDataPrepared = true;
}

//=============================================================================
// GPU-driven rendering path (G-Buffer)
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
    // Path selection: GPU Culling (Indirect Draw) vs CPU Instanced Draw
    // ========================================================================
    if (mUseGPUCulling && mCullPass && mLastCullOutput.indirectArgsBuffer)
    {
        // ---- GPU Culling Path: CS already dispatched, use Indirect Draw ----
        RenderGPUCulled(renderEncoder, cameraUBO, terrainGBufferPSO);
    }
    else
    {
        // ---- CPU Instanced Draw Path: original frustum cull + instanced draw ----
        RenderCPUInstanced(renderEncoder, cameraUBO, terrainGBufferPSO, frustum);
    }
}

//=============================================================================
// GPU Culled rendering path (Indirect Draw)
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

    // ---- Bind PSO and shared state ----
    renderEncoder->SetGraphicsPipeline(terrainGBufferPSO);
    if (mWireframe)
        renderEncoder->SetFillMode(FillModeWireframe);

    // UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // PatchMeta SSBO (ALL patches — GPU has already culled to indirect args)
    renderEncoder->SetStorageBuffer("gPatchMeta", allPatchMeta,
        RenderCore::ShaderStage_Vertex);

    // Heightmap texture (VS samples for Y displacement)
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // Template VB
    renderEncoder->SetVertexBuffer(mQuadTreeTerrain->GetTemplateVB(), 0, 0);

    // Material textures
    BindMaterialTextures(renderEncoder);

    // ---- Indirect Draw: GPU decided visibility per-patch ----
    renderEncoder->DrawIndexedPrimitivesIndirect(
        PrimitiveMode_TRIANGLES,
        templateIB,
        0,                                    // indexBufferOffset
        mLastCullOutput.indirectArgsBuffer,   // indirect args (GPU-written)
        0,                                    // indirectBufferOffset
        totalPatchCount,                      // drawCount (= total patches, culled ones have instanceCount=0)
        sizeof(RenderCore::DrawIndexedIndirectCommand)  // stride
    );
}

//=============================================================================
// CPU Instanced rendering path (original)
//=============================================================================

void TerrainComponent::RenderCPUInstanced(RenderEncoder* renderEncoder,
                                           UniformBufferPtr cameraUBO,
                                           GraphicsPipelinePtr terrainGBufferPSO,
                                           const Frustumf* frustum)
{
    // Prepare GPU path data (CPU frustum culling + visible PatchMeta SSBO)
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

    // ---- Bind PSO and shared state ----
    renderEncoder->SetGraphicsPipeline(terrainGBufferPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    // Bind UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // Bind PatchMeta SSBO to vertex stage (VS reads gPatchMeta[instanceID])
    renderEncoder->SetStorageBuffer("gPatchMeta", visiblePatchMeta,
        RenderCore::ShaderStage_Vertex);

    // Bind heightmap texture + sampler
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetVertexTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // Bind template mesh vertex buffers
    renderEncoder->SetVertexBuffer(templateVB, 0, 0);

    // Material textures
    BindMaterialTextures(renderEncoder);

    // ---- Instanced draw: one instance per visible patch ----
    renderEncoder->DrawIndexedInstancePrimitives(
        PrimitiveMode_TRIANGLES,
        1536,          // indexCount for 17x17 template mesh (16*16*6)
        templateIB,
        0,             // indexBufferOffset
        0,             // firstInstance
        visibleCount); // instanceCount = number of visible patches
}

//=============================================================================
// GPU-driven rendering path (Depth-only)
//=============================================================================

void TerrainComponent::RenderDepthOnly(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        UniformBufferPtr objectUBO,
                                        GraphicsPipelinePtr terrainDepthPSO,
                                        const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !renderEncoder || !terrainDepthPSO)
        return;

    // Path selection
    if (mUseGPUCulling && mCullPass && mLastCullOutput.indirectArgsBuffer)
    {
        // GPU Culling Path: Indirect Draw
        RenderDepthGPUCulled(renderEncoder, cameraUBO, terrainDepthPSO);
    }
    else
    {
        // CPU Instanced Draw Path
        RenderDepthCPUInstanced(renderEncoder, cameraUBO, terrainDepthPSO, frustum);
    }
}

//=============================================================================
// Depth-only GPU Culled path (Indirect Draw)
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
// Depth-only CPU Instanced path (original)
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
        1536,
        templateIB,
        0,
        0,
        visibleCount);
}

//=============================================================================
// Accessors
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
// GPU Culling toggle
//=============================================================================

void TerrainComponent::SetUseGPUCulling(bool enable)
{
    mUseGPUCulling = enable;

    if (enable && !mCullPass)
    {
        mCullPass = std::make_shared<TerrainCullPass>();
        if (!mCullPass->Initialize())
        {
            LOG_ERROR("TerrainComponent: Failed to initialize TerrainCullPass, falling back to CPU culling");
            mCullPass.reset();
            mUseGPUCulling = false;
        }
    }

    LOG_INFO("TerrainComponent: GPU culling %s", enable ? "enabled" : "disabled (using CPU instanced draw)");
}

//=============================================================================
// DispatchGPUCull - Execute GPU Compute Shader culling (call BEFORE Render)
//=============================================================================

void TerrainComponent::DispatchGPUCull(CommandBufferPtr commandBuffer,
                                        const mathutil::Matrix4x4f& vpMatrix,
                                        RenderCore::UniformBufferPtr cameraUBO)
{
    if (!mUseGPUCulling || !mCullPass || !mCullPass->IsInitialized() || !mQuadTreeTerrain)
    {
        // Clear cached output so Render() falls back to CPU path
        mLastCullOutput.indirectArgsBuffer = nullptr;
        mLastCullOutput.indirectArgsCount = 0;
        return;
    }

    // Build ALL patch meta buffer (not just visible — GPU does the culling)
    // Note: BuildPatchMetaBuffer() is called inside BuildGPUPathData or we call it directly
    // For GPU path, we need the full buffer from GetPatchMetaBuffer()
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

    // Dispatch compute shader culling
    mLastCullOutput = mCullPass->DispatchCull(commandBuffer, params, allPatchMeta, cameraUBO);

    LOG_DEBUG("TerrainComponent::DispatchGPUCull: %u patches, indirect args buffer %s",
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

    LOG_DEBUG("TerrainComponent::DispatchCullViaFrameGraph: %u patches registered in FrameGraph",
              totalPatchCount);
}

//=============================================================================
// BindMaterialTextures - Shared material texture binding (DRY helper)
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
