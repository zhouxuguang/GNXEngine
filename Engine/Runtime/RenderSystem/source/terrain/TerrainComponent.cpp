//
//  TerrainComponent.cpp
//  GNXEngine
//
//  Terrain rendering component with dedicated rendering path.
//  Uses GPU instanced drawing: single DrawIndexedInstancePrimitives call
//  with SSBO-based manual vertex fetch, replacing N per-leaf draw calls.
//

#include "terrain/TerrainComponent.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

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
// Instanced Rendering Initialization
//=============================================================================

void TerrainComponent::InitInstancedRendering()
{
    if (mInstancedInitialized || !mQuadTreeTerrain) return;

    auto device = RenderCore::GetRenderDevice();
    MeshPtr mesh = mQuadTreeTerrain->GetMesh();
    if (!mesh) return;

    uint32_t vertexCount = mesh->GetVertexCount();

    // ---- Create SSBOs for terrain vertex data ----
    // These wrap the same data as the existing VB but as StorageBuffers
    // so the instanced shader can fetch vertices manually.
    //
    // CRITICAL: Positions and normals use float4 (not float3) because MSL's
    // StructuredBuffer wraps elements in a struct where float3 has 16-byte
    // alignment. Using float4 ensures the GPU stride matches the upload stride.

    // Pad positions from float3 to float4 for MSL StructuredBuffer alignment
    {
        std::vector<Vector4f> paddedPos(vertexCount);
        auto posIter = mesh->GetPositionBegin();
        for (uint32_t i = 0; i < vertexCount; ++i, ++posIter)
            paddedPos[i] = Vector4f(posIter->x, posIter->y, posIter->z, 0.0f);
        RCBufferDesc posDesc(vertexCount * sizeof(Vector4f), RCBufferUsage::StorageBuffer, StorageModeShared);
        mSbPositions = device->CreateBuffer(posDesc, paddedPos.data());
    }

    // Pad normals from float3 to float4 for MSL StructuredBuffer alignment
    {
        std::vector<Vector4f> paddedNorm(vertexCount);
        auto normIter = mesh->GetNormalBegin<Vector3f>();
        for (uint32_t i = 0; i < vertexCount; ++i, ++normIter)
            paddedNorm[i] = Vector4f(normIter->x, normIter->y, normIter->z, 0.0f);
        RCBufferDesc normDesc(vertexCount * sizeof(Vector4f), RCBufferUsage::StorageBuffer, StorageModeShared);
        mSbNormals = device->CreateBuffer(normDesc, paddedNorm.data());
    }

    // Tangents are already float4 — no padding needed
    RCBufferDesc tanDesc(vertexCount * sizeof(Vector4f), RCBufferUsage::StorageBuffer, StorageModeShared);
    mSbTangents = device->CreateBuffer(tanDesc, mesh->GetTangentBegin<Vector4f>().operator->());

    // TexCoords are float2 — MSL alignment is 8 bytes, matches sizeof(Vector2f)
    RCBufferDesc uvDesc(vertexCount * sizeof(Vector2f), RCBufferUsage::StorageBuffer, StorageModeShared);
    mSbTexCoords = device->CreateBuffer(uvDesc, mesh->GetUvBegin().operator->());

    // Master index buffer as SSBO
    const std::vector<uint32_t>& indices = mesh->GetIndices();
    RCBufferDesc idxDesc((uint32_t)(indices.size() * sizeof(uint32_t)), RCBufferUsage::StorageBuffer, StorageModeShared);
    mSbIndices = device->CreateBuffer(idxDesc, indices.data());

    LOG_INFO("TerrainComponent: Created %u vertex SSBOs + index SSBO (%u indices)",
             vertexCount, (uint32_t)indices.size());

    // ---- Create instance data SSBO (StorageModeShared for CPU write each frame) ----
    // Size for worst case: every leaf is visible (reserve space)
    uint32_t maxPossiblePatches = mQuadTreeTerrain->GetGridSize(); // upper bound
    RCBufferDesc instanceDesc(maxPossiblePatches * sizeof(QuadTreeTerrain::PatchInstanceData),
                               RCBufferUsage::StorageBuffer, StorageModeShared);
    mSbInstances = device->CreateBuffer(instanceDesc);

    // ---- Create instanced PSOs ----
    GraphicsShaderInfo basePassShaderInfo = CreateGraphicsShaderInfo("TerrainBasePassInstanced");
    basePassShaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    basePassShaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionEqual;
    basePassShaderInfo.graphicsPipelineDesc.renderTargetCount = 5;
    mInstancedBasePassPSO = device->CreateGraphicsPipeline(basePassShaderInfo.graphicsPipelineDesc);
    mInstancedBasePassPSO->AttachGraphicsShader(basePassShaderInfo.graphicsShader);

    GraphicsShaderInfo depthShaderInfo = CreateGraphicsShaderInfo("TerrainDepthInstanced");
    depthShaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    mInstancedDepthPSO = device->CreateGraphicsPipeline(depthShaderInfo.graphicsPipelineDesc);
    mInstancedDepthPSO->AttachGraphicsShader(depthShaderInfo.graphicsShader);

    LOG_INFO("TerrainComponent: Instanced rendering initialized (BasePass + Depth PSOs)");

    // Pre-create dummy index buffer with uniform patch index count
    // (constant across all frames — set by BuildStaticIndexPool)
    uint32_t uniformCount = mQuadTreeTerrain->GetUniformPatchIndexCount();
    if (uniformCount > 0)
    {
        mDummyIndexBufferSize = uniformCount;
        std::vector<uint32_t> dummyIndices(uniformCount);
        for (uint32_t i = 0; i < uniformCount; ++i)
            dummyIndices[i] = i;
        mDummyIndexBuffer = RenderCore::GetRenderDevice()->CreateIndexBufferWithBytes(
            dummyIndices.data(), uniformCount * sizeof(uint32_t), IndexType_UInt);
    }

    mInstancedInitialized = true;
}

void TerrainComponent::UploadPatchInstanceData()
{
    if (!mSbInstances || !mQuadTreeTerrain) return;

    const auto& patches = mQuadTreeTerrain->GetPatchInstances();
    uint32_t patchCount = (uint32_t)patches.size();

    if (patchCount == 0) return;

    // Copy instance data to the shared SSBO via Map()
    void* ptr = mSbInstances->Map();
    if (ptr)
    {
        memcpy(ptr, patches.data(), patchCount * sizeof(QuadTreeTerrain::PatchInstanceData));
        mSbInstances->Unmap();
    }

    // Dummy IB is now pre-created in InitInstancedRendering() with uniform size.
    // No need to recreate it each frame since mUniformPatchIndexCount is constant.
}

//=============================================================================
// Dedicated terrain rendering path — GPU Instanced
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

    // Lazy-init instanced resources on first render call
    if (!mInstancedInitialized)
    {
        InitInstancedRendering();
    }

    MeshPtr mesh = mQuadTreeTerrain->GetMesh();
    if (!mesh) return;

    // Build patch instance data (with frustum culling)
    mQuadTreeTerrain->BuildPatchInstances(frustum);

    uint32_t patchCount = mQuadTreeTerrain->GetPatchInstanceCount();
    if (patchCount == 0) return;

    // Upload instance data to GPU + ensure dummy IB is ready
    UploadPatchInstanceData();

    if (!mDummyIndexBuffer || !mInstancedBasePassPSO) return;

    // ---- Bind instanced BasePass PSO ----
    renderEncoder->SetGraphicsPipeline(mInstancedBasePassPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    // Bind UBOs (same as non-instanced path)
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbPerObject", objectUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // ---- Bind terrain SSBOs (vertex data) ----
    renderEncoder->SetStorageBuffer("_TerrainPositions", mSbPositions, ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_TerrainNormals",   mSbNormals,   ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_TerrainTangents",  mSbTangents,  ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_TerrainTexCoords", mSbTexCoords, ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_TerrainIndices",    mSbIndices,   ShaderStage::ShaderStage_Vertex);

    // Bind per-patch instance data SSBO
    renderEncoder->SetStorageBuffer("_PatchInstances", mSbInstances, ShaderStage::ShaderStage_Vertex);

    // ---- Bind material textures (same as non-instanced) ----
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

    // ---- Single instanced draw call for ALL patches ----
    renderEncoder->DrawIndexedInstancePrimitives(
        PrimitiveMode_TRIANGLES,
        (int)mDummyIndexBufferSize,
        mDummyIndexBuffer,
        0,
        0,
        patchCount);
}

//=============================================================================
// Depth-only rendering — GPU Instanced
//=============================================================================

void TerrainComponent::RenderDepthOnly(RenderEncoder* renderEncoder,
                                        UniformBufferPtr cameraUBO,
                                        UniformBufferPtr objectUBO,
                                        GraphicsPipelinePtr depthPSO,
                                        const Frustumf* frustum)
{
    if (!mQuadTreeTerrain || !renderEncoder)
    {
        return;
    }

    // Lazy-init instanced resources on first render call
    if (!mInstancedInitialized)
    {
        InitInstancedRendering();
    }

    MeshPtr mesh = mQuadTreeTerrain->GetMesh();
    if (!mesh || !mesh->HasChannel(kShaderChannelPosition)) return;

    // Build patch instance data (with frustum culling)
    mQuadTreeTerrain->BuildPatchInstances(frustum);

    uint32_t patchCount = mQuadTreeTerrain->GetPatchInstanceCount();
    if (patchCount == 0) return;

    // Upload instance data to GPU + ensure dummy IB is ready
    UploadPatchInstanceData();

    if (!mDummyIndexBuffer || !mInstancedDepthPSO) return;

    // ---- Bind instanced Depth PSO ----
    renderEncoder->SetGraphicsPipeline(mInstancedDepthPSO);

    if (mWireframe)
    {
        renderEncoder->SetFillMode(FillModeWireframe);
    }

    // Bind UBOs
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    renderEncoder->SetVertexUniformBuffer("cbPerObject", objectUBO);
    renderEncoder->SetFragmentUniformBuffer("cbPerCamera", cameraUBO);

    // ---- Bind terrain SSBOs (depth only needs positions + indices + instances) ----
    renderEncoder->SetStorageBuffer("_TerrainPositions", mSbPositions, ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_TerrainIndices",    mSbIndices,   ShaderStage::ShaderStage_Vertex);
    renderEncoder->SetStorageBuffer("_PatchInstances", mSbInstances, ShaderStage::ShaderStage_Vertex);

    // ---- Single instanced draw call for ALL patches ----
    renderEncoder->DrawIndexedInstancePrimitives(
        PrimitiveMode_TRIANGLES,
        (int)mDummyIndexBufferSize,
        mDummyIndexBuffer,
        0,
        0,
        patchCount);
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
