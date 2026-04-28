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

    // Prepare GPU path data (culling + visible PatchMeta)
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
        renderEncoder->SetFillMode(FillModeWireframe);

    // Bind UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // Bind PatchMeta SSBO to vertex stage (VS reads gPatchMeta[instanceID])
    renderEncoder->SetStorageBuffer("gPatchMeta", visiblePatchMeta,
        RenderCore::ShaderStage_Vertex);

    // Bind heightmap texture (accessible from VS via shared descriptor)
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetFragmentTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // Bind template mesh vertex buffers (SoA layout: positions | texCoords)
    uint32_t posSize = mQuadTreeTerrain->GetTemplatePositionSize();
    renderEncoder->SetVertexBuffer(templateVB, 0, 0);       // positions at offset 0
    renderEncoder->SetVertexBuffer(templateVB, posSize, 1);  // texCoords at offset posSize

    // Bind material textures
    TextureSamplerPtr textureSampler;
    if (auto mesh = mQuadTreeTerrain->GetMesh())
        textureSampler = mesh->GetSampler();
    if (!textureSampler)
    {
        RenderCore::SamplerDesc defaultDesc;
        defaultDesc.filterMag = RenderCore::MAG_LINEAR;
        defaultDesc.filterMin = RenderCore::MIN_LINEAR;  // Linear filtering for min
        defaultDesc.filterMip = RenderCore::MIN_LINEAR_MIPMAP_LINEAR;  // Trilinear filtering
        defaultDesc.anisotropyLog2 = 2;  // Enable 4x anisotropic filtering for better scaling
        defaultDesc.wrapS = RenderCore::REPEAT;  // Repeat texture for tiling
        defaultDesc.wrapT = RenderCore::REPEAT;  // Repeat texture for tiling
        textureSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(defaultDesc);
    }

    auto diffuseSlot = mMaterial->GetTextureSlot("diffuseTexture");
    if (diffuseSlot)
    {
        // Create optimized sampler with anisotropic filtering for better scaling
        RenderCore::SamplerDesc optimizedDesc = diffuseSlot->samplerDesc;
        optimizedDesc.filterMin = RenderCore::MIN_LINEAR;
        optimizedDesc.filterMip = RenderCore::MIN_LINEAR_MIPMAP_LINEAR;
        optimizedDesc.anisotropyLog2 = 2;  // Enable 4x anisotropic filtering
        optimizedDesc.wrapS = RenderCore::REPEAT;
        optimizedDesc.wrapT = RenderCore::REPEAT;
        
        auto sampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(optimizedDesc);
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", diffuseSlot->texture, sampler);
    }
    else
    {
        // If no diffuse texture, bind a null texture to ensure binding consistency
        // Shader will handle the fallback to a default color
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", nullptr, textureSampler);
    }

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

    // Prepare GPU path data (culling + visible PatchMeta)
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
    renderEncoder->SetGraphicsPipeline(terrainDepthPSO);

    if (mWireframe)
        renderEncoder->SetFillMode(FillModeWireframe);

    // Bind UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbTerrain", mTerrainParamsUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // Bind PatchMeta SSBO to vertex stage
    renderEncoder->SetStorageBuffer("gPatchMeta", visiblePatchMeta,
        RenderCore::ShaderStage_Vertex);

    // Bind heightmap texture
    RenderCore::SamplerDesc heightmapSamplerDesc(
        RenderCore::MAG_LINEAR, RenderCore::MIN_LINEAR,
        RenderCore::CLAMP_TO_EDGE, RenderCore::CLAMP_TO_EDGE);
    auto heightmapSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(heightmapSamplerDesc);
    renderEncoder->SetFragmentTextureAndSampler("gHeightmap", heightmapTexture, heightmapSampler);

    // Bind template mesh vertex buffers
    uint32_t posSize = mQuadTreeTerrain->GetTemplatePositionSize();
    renderEncoder->SetVertexBuffer(templateVB, 0, 0);       // positions at offset 0
    renderEncoder->SetVertexBuffer(templateVB, posSize, 1);  // texCoords at offset posSize

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

NS_RENDERSYSTEM_END
