//
//  TerrainComponent.cpp
//  GNXEngine
//
//  Terrain rendering component with dedicated rendering path.
//  Binds PSO/material once and draws all visible patches in a loop,
//  avoiding the per-submesh PSO rebind overhead of MeshDrawUtil.
//

#include "terrain/TerrainComponent.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/BaseLib/include/LogService.h"

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
                                          uint32_t patchSize)
{
    mGeoMipTerrain = GeoMipTerrain::CreateFromHeightMap(
        heightmapPath, worldSizeXZ, heightScale, patchSize);

    if (mGeoMipTerrain)
    {
        LOG_INFO("TerrainComponent: Initialized from heightmap '%s', %u patches, %u LOD levels",
                 heightmapPath, mGeoMipTerrain->GetPatchCount(), mGeoMipTerrain->GetMaxLOD() + 1);
    }
    else
    {
        LOG_ERROR("TerrainComponent: Failed to load heightmap '%s'", heightmapPath);
    }
}

void TerrainComponent::InitProcedural(uint32_t gridSize,
                                       float worldSizeXZ,
                                       float heightScale,
                                       uint32_t patchSize)
{
    mGeoMipTerrain = GeoMipTerrain::Create(
        gridSize, worldSizeXZ, heightScale, patchSize);

    if (mGeoMipTerrain)
    {
        LOG_INFO("TerrainComponent: Initialized procedural terrain, %u patches, %u LOD levels",
                 mGeoMipTerrain->GetPatchCount(), mGeoMipTerrain->GetMaxLOD() + 1);
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
    // LOD update is driven by camera position; the scene system or demo
    // should call UpdateLOD explicitly when the camera moves.
    // This Update hook is reserved for future use (e.g., vegetation animation).
}

//=============================================================================
// Dedicated terrain rendering path
//=============================================================================

void TerrainComponent::Render(RenderEncoder* renderEncoder,
                               UniformBufferPtr cameraUBO,
                               UniformBufferPtr objectUBO,
                               GraphicsPipelinePtr basePassPSO)
{
    if (!mGeoMipTerrain || !mMaterial || !renderEncoder)
    {
        return;
    }

    MeshPtr mesh = mGeoMipTerrain->GetMesh();
    if (!mesh)
    {
        return;
    }

    const ChannelInfo* channels = mesh->GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh->GetVertexBuffer();
    IndexBufferPtr  indexBuffer  = mesh->GetIndexBuffer();

    if (!vertexBuffer || !indexBuffer)
    {
        return;
    }

    // ---- Bind PSO and shared state ONCE for all patches ----
    renderEncoder->SetGraphicsPipeline(basePassPSO);

    // 应用线框模式
    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbPerObject", objectUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // ---- Bind vertex buffers ONCE ----
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);

    // ---- Bind material textures ONCE ----
    TextureSamplerPtr textureSampler = mesh->GetSampler();
    if (!textureSampler)
    {
        SamplerDesc defaultDesc;
        textureSampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(defaultDesc);
    }

    auto diffuseSlot = mMaterial->GetTextureSlot("diffuseTexture");
    if (diffuseSlot)
    {
        SamplerDesc desc = diffuseSlot->samplerDesc;
        auto sampler = RenderCore::GetRenderDevice()->CreateSamplerWithDescriptor(desc);
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", diffuseSlot->texture, sampler);
    }

    renderEncoder->SetFragmentTextureAndSampler("gNormalMap",     mMaterial->GetTexture("normalTexture"),   textureSampler);
    renderEncoder->SetFragmentTextureAndSampler("gMetalRoughMap", mMaterial->GetTexture("roughnessTexture"), textureSampler);
    renderEncoder->SetFragmentTextureAndSampler("gEmissiveMap",   mMaterial->GetTexture("emissiveTexture"),  textureSampler);
    renderEncoder->SetFragmentTextureAndSampler("gAmbientMap",    mMaterial->GetTexture("ambientTexture"),   textureSampler);

    // ---- Draw all visible patches ----
    int subMeshCount = mesh->GetSubMeshCount();
    for (int n = 0; n < subMeshCount; n++)
    {
        const SubMeshInfo& subInfo = mesh->GetSubMeshInfo(n);

        // TODO: per-patch frustum culling here
        // if (!IsPatchVisible(subInfo, frustum)) continue;

        renderEncoder->DrawIndexedPrimitives(
            subInfo.topology,
            (int)subInfo.indexCount,
            indexBuffer,
            subInfo.firstIndex,
            subInfo.baseVertex);
    }
}

//=============================================================================
// Accessors
//=============================================================================

void TerrainComponent::RenderDepthOnly(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        UniformBufferPtr objectUBO,
                                        GraphicsPipelinePtr depthPSO)
{
    if (!mGeoMipTerrain || !renderEncoder || !depthPSO)
    {
        return;
    }

    MeshPtr mesh = mGeoMipTerrain->GetMesh();
    if (!mesh || !mesh->HasChannel(kShaderChannelPosition))
    {
        return;
    }

    const ChannelInfo* channels = mesh->GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh->GetVertexBuffer();
    IndexBufferPtr  indexBuffer  = mesh->GetIndexBuffer();

    if (!vertexBuffer || !indexBuffer)
    {
        return;
    }

    // ---- Bind depth PSO and shared state ONCE ----
    renderEncoder->SetGraphicsPipeline(depthPSO);

    // 应用线框模式
    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbPerObject", objectUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // ---- Only bind position vertex buffer ----
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);

    // ---- Draw all patches ----
    int subMeshCount = mesh->GetSubMeshCount();
    for (int n = 0; n < subMeshCount; n++)
    {
        const SubMeshInfo& subInfo = mesh->GetSubMeshInfo(n);
        renderEncoder->DrawIndexedPrimitives(
            subInfo.topology,
            (int)subInfo.indexCount,
            indexBuffer,
            subInfo.firstIndex,
            subInfo.baseVertex);
    }
}

//=============================================================================
// Accessors (continued)
//=============================================================================

float TerrainComponent::GetHeight(float worldX, float worldZ) const
{
    if (!mGeoMipTerrain)
    {
        return 0.0f;
    }
    return mGeoMipTerrain->GetHeight(worldX, worldZ);
}

void TerrainComponent::SetLODDistances(const std::vector<float>& distances)
{
    if (mGeoMipTerrain)
    {
        mGeoMipTerrain->SetLODDistances(distances);
    }
}

void TerrainComponent::SetWireframe(bool wireframe)
{
    mWireframe = wireframe;
}

NS_RENDERSYSTEM_END
