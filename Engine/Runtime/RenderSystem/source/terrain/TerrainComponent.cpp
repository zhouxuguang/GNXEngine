//
//  TerrainComponent.cpp
//  GNXEngine
//
//  Terrain rendering component with dedicated rendering path.
//  Binds PSO/material once and draws all visible leaf nodes,
//  avoiding the per-submesh PSO rebind overhead of MeshDrawUtil.
//

#include "terrain/TerrainComponent.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
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
    // LOD update is driven by camera position; the scene system or demo
    // should call QuadTreeTerrain::Update explicitly when the camera moves.
    // This Update hook is reserved for future use (e.g., vegetation animation).
}

//=============================================================================
// Dedicated terrain rendering path
//=============================================================================

void TerrainComponent::Render(RenderEncoder* renderEncoder,
                               UniformBufferPtr cameraUBO,
                               UniformBufferPtr objectUBO,
                               GraphicsPipelinePtr basePassPSO,
                               const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !mMaterial || !renderEncoder)
    {
        return;
    }

    MeshPtr mesh = mQuadTreeTerrain->GetMesh();
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

    // ---- Build indirect draw commands (includes frustum culling) ----
    mQuadTreeTerrain->BuildIndirectCommands(frustum);
    uint32_t drawCount = mQuadTreeTerrain->GetIndirectDrawCount();

    if (drawCount == 0)
    {
        return;
    }

    // ---- Ensure indirect buffer is large enough ----
    const auto& indirectCommands = mQuadTreeTerrain->GetIndirectCommands();
    uint32_t requiredSize = drawCount * sizeof(RenderCore::DrawIndexedIndirectCommand);

    if (!mIndirectBuffer || mIndirectBufferSize < requiredSize)
    {
        // Allocate with 50% extra room to avoid frequent reallocation
        uint32_t allocSize = requiredSize + requiredSize / 2;
        RenderCore::RCBufferDesc desc(allocSize,
            RenderCore::RCBufferUsage::IndirectBuffer,
            RenderCore::StorageMode::StorageModeShared);
        mIndirectBuffer = RenderCore::GetRenderDevice()->CreateBuffer(desc);
        mIndirectBufferSize = allocSize;
        if (mIndirectBuffer)
        {
            mIndirectBuffer->SetName("TerrainIndirectBuffer");
        }
    }

    // ---- Upload indirect commands to GPU ----
    if (mIndirectBuffer)
    {
        void* mapped = mIndirectBuffer->Map();
        if (mapped)
        {
            memcpy(mapped, indirectCommands.data(), requiredSize);
            mIndirectBuffer->Unmap();
        }
    }

    // ---- Bind PSO and shared state ONCE for all leaf nodes ----
    renderEncoder->SetGraphicsPipeline(basePassPSO);

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

    // ---- Single indirect draw call for ALL visible leaf nodes ----
    renderEncoder->DrawIndexedPrimitivesIndirect(
        PrimitiveMode_TRIANGLES,
        indexBuffer,
        0,  // indexBufferOffset - bind the entire index buffer
        mIndirectBuffer,
        0,  // indirectBufferOffset
        drawCount,
        sizeof(RenderCore::DrawIndexedIndirectCommand));
}

//=============================================================================
// Depth-only rendering
//=============================================================================

void TerrainComponent::RenderDepthOnly(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        UniformBufferPtr objectUBO,
                                        GraphicsPipelinePtr depthPSO,
                                        const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !renderEncoder || !depthPSO)
    {
        return;
    }

    MeshPtr mesh = mQuadTreeTerrain->GetMesh();
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

    // ---- Build indirect draw commands (includes frustum culling) ----
    mQuadTreeTerrain->BuildIndirectCommands(frustum);
    uint32_t drawCount = mQuadTreeTerrain->GetIndirectDrawCount();

    if (drawCount == 0)
    {
        return;
    }

    // ---- Ensure indirect buffer is large enough ----
    const auto& indirectCommands = mQuadTreeTerrain->GetIndirectCommands();
    uint32_t requiredSize = drawCount * sizeof(RenderCore::DrawIndexedIndirectCommand);

    if (!mIndirectBuffer || mIndirectBufferSize < requiredSize)
    {
        uint32_t allocSize = requiredSize + requiredSize / 2;
        RenderCore::RCBufferDesc desc(allocSize,
            RenderCore::RCBufferUsage::IndirectBuffer,
            RenderCore::StorageMode::StorageModeShared);
        mIndirectBuffer = RenderCore::GetRenderDevice()->CreateBuffer(desc);
        mIndirectBufferSize = allocSize;
        if (mIndirectBuffer)
        {
            mIndirectBuffer->SetName("TerrainIndirectBuffer");
        }
    }

    // ---- Upload indirect commands to GPU ----
    if (mIndirectBuffer)
    {
        void* mapped = mIndirectBuffer->Map();
        if (mapped)
        {
            memcpy(mapped, indirectCommands.data(), requiredSize);
            mIndirectBuffer->Unmap();
        }
    }

    // ---- Bind depth PSO and shared state ONCE ----
    renderEncoder->SetGraphicsPipeline(depthPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbPerObject", objectUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // ---- Only bind position vertex buffer ----
    renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);

    // ---- Single indirect draw call for ALL visible leaf nodes ----
    renderEncoder->DrawIndexedPrimitivesIndirect(
        PrimitiveMode_TRIANGLES,
        indexBuffer,
        0,
        mIndirectBuffer,
        0,
        drawCount,
        sizeof(RenderCore::DrawIndexedIndirectCommand));
}

//=============================================================================
// Accessors
//=============================================================================

float TerrainComponent::GetHeight(float worldX, float worldZ) const
{
    if (!mQuadTreeTerrain)
    {
        return 0.0f;
    }
    return mQuadTreeTerrain->GetHeight(worldX, worldZ);
}

void TerrainComponent::SetLODDistanceFactor(float factor)
{
    if (mQuadTreeTerrain)
    {
        mQuadTreeTerrain->SetLODDistanceFactor(factor);
    }
}

void TerrainComponent::SetSSEThreshold(float threshold)
{
    if (mQuadTreeTerrain)
    {
        mQuadTreeTerrain->SetSSEThreshold(threshold);
    }
}

void TerrainComponent::SetWireframe(bool wireframe)
{
    mWireframe = wireframe;
}

NS_RENDERSYSTEM_END
