//
//  GeoMipTerrain.cpp
//  GNXEngine
//
//  GeoMipMapping terrain with LOD support and crack fixing.
//  Uses a static master index buffer with per-patch baseVertex draws,
//  matching the ogldev Terrain7 approach.
//  The index buffer is built once at init time and NEVER changes.
//  UpdateLOD only updates per-patch SubMeshInfo entries (firstIndex,
//  indexCount, baseVertex) — no per-frame GPU index buffer upload.
//

#include "terrain/GeoMipTerrain.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <cmath>
#include <algorithm>
#include <vector>

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//=============================================================================
// Procedural height function (multi-octave sine-wave noise)
//=============================================================================

float GeoMipTerrain::ComputeHeight(float x, float z)
{
    float h = 0.0f;
    h += sinf(x * 0.01f + 0.5f) * cosf(z * 0.013f + 1.3f) * 40.0f;
    h += sinf(x * 0.03f + 2.1f) * cosf(z * 0.027f + 0.7f) * 20.0f;
    h += sinf(x * 0.07f + 5.3f) * cosf(z * 0.061f + 3.1f) * 10.0f;
    h += sinf(x * 0.13f + 1.7f) * cosf(z * 0.17f + 4.2f) * 5.0f;
    return h;
}

//=============================================================================
// Integer power helper
//=============================================================================

uint32_t GeoMipTerrain::PowInt(uint32_t base, uint32_t exp)
{
    uint32_t result = 1;
    for (uint32_t i = 0; i < exp; ++i)
        result *= base;
    return result;
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

GeoMipTerrain::GeoMipTerrain() = default;
GeoMipTerrain::~GeoMipTerrain() = default;

//=============================================================================
// Factory: Create from heightmap image
//=============================================================================

GeoMipTerrainPtr GeoMipTerrain::CreateFromHeightMap(
    const char* heightmapPath,
    float worldSizeXZ,
    float heightScale,
    uint32_t patchSize)
{
    using namespace imagecodec;

    // Load heightmap
    VImage heightmapImage;
    if (!ImageDecoder::DecodeFile(heightmapPath, &heightmapImage))
    {
        LOG_ERROR("GeoMipTerrain: Failed to load heightmap: %s", heightmapPath);
        return nullptr;
    }

    if (heightmapImage.GetFormat() != FORMAT_GRAY8 && heightmapImage.GetFormat() != FORMAT_GRAY16)
    {
        LOG_ERROR("GeoMipTerrain: Heightmap must be GRAY8 or GRAY16, got format %d",
                  heightmapImage.GetFormat());
        return nullptr;
    }

    uint32_t imageWidth  = heightmapImage.GetWidth();
    uint32_t imageHeight = heightmapImage.GetHeight();
    if (imageWidth != imageHeight)
    {
        LOG_WARN("GeoMipTerrain: Heightmap is not square (%ux%u), using width", imageWidth, imageHeight);
    }

    // Compute grid size: round up to nearest valid size
    // (gridSize - 1) must be divisible by (patchSize - 1)
    uint32_t patchStride = patchSize - 1;
    uint32_t numPatches  = ((imageWidth - 1) + patchStride - 1) / patchStride; // ceil
    uint32_t gridSize    = numPatches * patchStride + 1;

    auto terrain = GeoMipTerrainPtr(new GeoMipTerrain());
    terrain->mWorldSize       = worldSizeXZ;
    terrain->mHeightScale     = heightScale;
    terrain->mPatchSize       = patchSize;
    terrain->mGridSize        = gridSize;
    terrain->mPatchesPerSide  = numPatches;

    LOG_INFO("GeoMipTerrain: Heightmap %s (%ux%u), grid=%u, patches=%ux%u, patchSize=%u",
             heightmapPath, imageWidth, imageHeight, gridSize, numPatches, numPatches, patchSize);

    // Sample heightmap into mHeightMap
    uint32_t vertexCount = gridSize * gridSize;
    terrain->mHeightMap.resize(vertexCount);

    uint8_t* imgData  = heightmapImage.GetImageData();
    uint32_t rowBytes = heightmapImage.GetBytesPerRow();
    bool isGray16     = (heightmapImage.GetFormat() == FORMAT_GRAY16);

    for (uint32_t iz = 0; iz < gridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < gridSize; ++ix)
        {
            uint32_t imgX = std::min(ix, imageWidth - 1);
            uint32_t imgZ = std::min(iz, imageHeight - 1);

            float h;
            if (isGray16)
            {
                uint16_t val = *(uint16_t*)(imgData + imgZ * rowBytes + imgX * 2);
                h = (float)val;
            }
            else
            {
                h = (float)imgData[imgZ * rowBytes + imgX];
            }

            terrain->mHeightMap[iz * gridSize + ix] = h * heightScale;
        }
    }

    // Generate vertices
    float step     = worldSizeXZ / (float)(gridSize - 1);
    float halfSize = worldSizeXZ * 0.5f;

    std::vector<Vector3f> positions(vertexCount);
    std::vector<Vector3f> normals(vertexCount);
    std::vector<Vector4f> tangents(vertexCount);
    std::vector<Vector2f> uvs(vertexCount);

    for (uint32_t iz = 0; iz < gridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < gridSize; ++ix)
        {
            uint32_t idx = iz * gridSize + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;
            float y = terrain->mHeightMap[idx];

            positions[idx] = Vector3f(x, y, z);
            uvs[idx]       = Vector2f((float)ix / (float)(gridSize - 1),
                                      (float)iz / (float)(gridSize - 1));
        }
    }

    // Compute normals via central differences on the heightmap
    for (uint32_t iz = 0; iz < gridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < gridSize; ++ix)
        {
            uint32_t idx = iz * gridSize + ix;

            float hL = terrain->mHeightMap[iz * gridSize + (ix > 0 ? ix - 1 : ix)];
            float hR = terrain->mHeightMap[iz * gridSize + (ix < gridSize - 1 ? ix + 1 : ix)];
            float hD = terrain->mHeightMap[(iz > 0 ? iz - 1 : iz) * gridSize + ix];
            float hU = terrain->mHeightMap[(iz < gridSize - 1 ? iz + 1 : iz) * gridSize + ix];

            Vector3f e1(2.0f * step, hR - hL, 0.0f);
            Vector3f e2(0.0f, hU - hD, 2.0f * step);
            normals[idx] = Vector3f::CrossProduct(e2, e1).Normalize();
        }
    }

    // Compute tangents from X-direction height derivative
    for (uint32_t iz = 0; iz < gridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < gridSize; ++ix)
        {
            uint32_t idx = iz * gridSize + ix;

            float hL = terrain->mHeightMap[iz * gridSize + (ix > 0 ? ix - 1 : ix)];
            float hR = terrain->mHeightMap[iz * gridSize + (ix < gridSize - 1 ? ix + 1 : ix)];
            float dhdx = (hR - hL) / (2.0f * step);

            Vector3f tan3 = Vector3f(1.0f, dhdx, 0.0f).Normalize();
            tangents[idx] = Vector4f(tan3.x, tan3.y, tan3.z, 1.0f);
        }
    }

    terrain->InitVertexData(positions, normals, tangents, uvs);
    terrain->GenerateLODIndexTemplates();

    // Set default LOD distances
    float patchWorldSize = worldSizeXZ / (float)numPatches;
    std::vector<float> defaultDistances;
    for (uint32_t lod = 0; lod <= terrain->mMaxLOD; ++lod)
    {
        defaultDistances.push_back(patchWorldSize * (float)(1 << lod));
    }
    terrain->SetLODDistances(defaultDistances);

    // Initial full-detail build
    terrain->UpdateLOD(Vector3f(0, 0, 0));

    LOG_INFO("GeoMipTerrain: Created from heightmap, %u patches, %u LOD levels",
             terrain->mPatches.size(), terrain->mMaxLOD + 1);

    return terrain;
}

//=============================================================================
// Factory: Create from procedural noise
//=============================================================================

GeoMipTerrainPtr GeoMipTerrain::Create(
    uint32_t gridSize,
    float worldSizeXZ,
    float heightScale,
    uint32_t patchSize)
{
    // Adjust gridSize to satisfy (gridSize - 1) % (patchSize - 1) == 0
    uint32_t patchStride     = patchSize - 1;
    uint32_t numPatches      = (gridSize - 1) / patchStride;
    if (numPatches == 0) numPatches = 1;
    uint32_t adjustedGridSize = numPatches * patchStride + 1;

    auto terrain = GeoMipTerrainPtr(new GeoMipTerrain());
    terrain->mWorldSize      = worldSizeXZ;
    terrain->mHeightScale    = heightScale;
    terrain->mPatchSize      = patchSize;
    terrain->mGridSize       = adjustedGridSize;
    terrain->mPatchesPerSide = numPatches;

    uint32_t vertexCount = adjustedGridSize * adjustedGridSize;
    float step           = worldSizeXZ / (float)(adjustedGridSize - 1);
    float halfSize       = worldSizeXZ * 0.5f;

    terrain->mHeightMap.resize(vertexCount);

    std::vector<Vector3f>  positions(vertexCount);
    std::vector<Vector3f>  normals(vertexCount);
    std::vector<Vector4f>  tangents(vertexCount);
    std::vector<Vector2f>  uvs(vertexCount);

    // Generate vertices with procedural height
    for (uint32_t iz = 0; iz < adjustedGridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < adjustedGridSize; ++ix)
        {
            uint32_t idx = iz * adjustedGridSize + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;
            float y = ComputeHeight(x, z) * heightScale;

            terrain->mHeightMap[idx] = y;
            positions[idx] = Vector3f(x, y, z);
            uvs[idx]       = Vector2f((float)ix / (float)(adjustedGridSize - 1),
                                      (float)iz / (float)(adjustedGridSize - 1));
        }
    }

    // Compute normals via central differences
    for (uint32_t iz = 0; iz < adjustedGridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < adjustedGridSize; ++ix)
        {
            uint32_t idx = iz * adjustedGridSize + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;

            float hL = ComputeHeight(x - step, z) * heightScale;
            float hR = ComputeHeight(x + step, z) * heightScale;
            float hD = ComputeHeight(x, z - step) * heightScale;
            float hU = ComputeHeight(x, z + step) * heightScale;

            Vector3f e1(2.0f * step, hR - hL, 0.0f);
            Vector3f e2(0.0f, hU - hD, 2.0f * step);
            normals[idx] = Vector3f::CrossProduct(e2, e1).Normalize();
        }
    }

    // Compute tangents
    for (uint32_t iz = 0; iz < adjustedGridSize; ++iz)
    {
        for (uint32_t ix = 0; ix < adjustedGridSize; ++ix)
        {
            uint32_t idx = iz * adjustedGridSize + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;

            float hR = ComputeHeight(x + step, z) * heightScale;
            float hL = ComputeHeight(x - step, z) * heightScale;
            float dhdx = (hR - hL) / (2.0f * step);

            Vector3f tan3 = Vector3f(1.0f, dhdx, 0.0f).Normalize();
            tangents[idx] = Vector4f(tan3.x, tan3.y, tan3.z, 1.0f);
        }
    }

    terrain->InitVertexData(positions, normals, tangents, uvs);
    terrain->GenerateLODIndexTemplates();

    // Set default LOD distances
    float patchWorldSize = worldSizeXZ / (float)numPatches;
    std::vector<float> defaultDistances;
    for (uint32_t lod = 0; lod <= terrain->mMaxLOD; ++lod)
    {
        defaultDistances.push_back(patchWorldSize * (float)(1 << lod));
    }
    terrain->SetLODDistances(defaultDistances);

    // Initial full-detail build
    terrain->UpdateLOD(Vector3f(0, 0, 0));

    LOG_INFO("GeoMipTerrain: Created procedural, grid=%u, %u patches, %u LOD levels",
             adjustedGridSize, terrain->mPatches.size(), terrain->mMaxLOD + 1);

    return terrain;
}

//=============================================================================
// Initialize vertex data and create GPU vertex buffer
//=============================================================================

void GeoMipTerrain::InitVertexData(
    const std::vector<Vector3f>& positions,
    const std::vector<Vector3f>& normals,
    const std::vector<Vector4f>& tangents,
    const std::vector<Vector2f>& uvs)
{
    uint32_t vertexCount = mGridSize * mGridSize;

    mMesh = std::make_shared<Mesh>();

    VertexData& vertexData = mMesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f); // 48
    vertexData.Resize(vertexCount, stride);

    ChannelInfo* channels = vertexData.GetChannels();

    channels[kShaderChannelPosition].offset = 0;
    channels[kShaderChannelPosition].format = VertexFormatFloat3;
    channels[kShaderChannelPosition].stride = sizeof(Vector3f);

    channels[kShaderChannelTangent].offset = (uint32_t)(positions.size() * sizeof(Vector3f));
    channels[kShaderChannelTangent].format = VertexFormatFloat4;
    channels[kShaderChannelTangent].stride = sizeof(Vector4f);

    channels[kShaderChannelNormal].offset = (uint32_t)(
        positions.size() * sizeof(Vector3f) +
        tangents.size() * sizeof(Vector4f));
    channels[kShaderChannelNormal].format = VertexFormatFloat3;
    channels[kShaderChannelNormal].stride = sizeof(Vector3f);

    channels[kShaderChannelTexCoord0].offset = (uint32_t)(
        positions.size() * sizeof(Vector3f) +
        tangents.size() * sizeof(Vector4f) +
        normals.size() * sizeof(Vector3f));
    channels[kShaderChannelTexCoord0].format = VertexFormatFloat2;
    channels[kShaderChannelTexCoord0].stride = sizeof(Vector2f);

    mMesh->SetPositions(positions.data(), vertexCount);
    mMesh->SetTangents(tangents.data(), vertexCount);
    mMesh->SetNormals(normals.data(), vertexCount);
    mMesh->SetUv(0, uvs.data(), vertexCount);

    // Vertex buffer is created here; index buffer will be created
    // by GenerateLODIndexTemplates() since it's a static master buffer.
    mMesh->SetUpBuffer();
}

//=============================================================================
// CreateTriangleFan - generate triangles for a single LOD cell with
// crack fixing based on neighbor LOD levels.
//
// Generates 4-8 triangles centered on the cell's center vertex.
// Each of the 4 sectors (up/right/down/left) has:
//   - A "first" triangle (always generated)
//   - A "second" triangle (only if that neighbor's LOD == core LOD)
// When a neighbor is coarser (lodNeighbor > lodCore), we use the
// neighbor's larger step and skip the second triangle to avoid T-junctions.
//
// All indices use the global row stride (gridSize) so that baseVertex
// (patchStartZ * gridSize + patchStartX) correctly offsets them to
// the right vertex in the vertex buffer.
//=============================================================================

void GeoMipTerrain::CreateTriangleFan(
    std::vector<uint32_t>& indices,
    uint32_t lodCore,
    uint32_t lodLeft,
    uint32_t lodRight,
    uint32_t lodTop,
    uint32_t lodBottom,
    uint32_t x,
    uint32_t z,
    uint32_t stride)
{
    uint32_t StepLeft   = PowInt(2, lodLeft);
    uint32_t StepRight  = PowInt(2, lodRight);
    uint32_t StepTop    = PowInt(2, lodTop);
    uint32_t StepBottom = PowInt(2, lodBottom);
    uint32_t StepCenter = PowInt(2, lodCore);

    uint32_t IndexCenter = (z + StepCenter) * stride + x + StepCenter;

    // first up (left edge of top sector)
    uint32_t IndexTemp1 = z * stride + x;
    uint32_t IndexTemp2 = (z + StepLeft) * stride + x;
    indices.push_back(IndexCenter);
    indices.push_back(IndexTemp1);
    indices.push_back(IndexTemp2);

    // second up (only if left neighbor has same LOD as core)
    if (lodLeft == lodCore)
    {
        IndexTemp1 = IndexTemp2;
        IndexTemp2 += StepLeft * stride;
        indices.push_back(IndexCenter);
        indices.push_back(IndexTemp1);
        indices.push_back(IndexTemp2);
    }

    // first right (top edge of right sector)
    IndexTemp1 = IndexTemp2;
    IndexTemp2 += StepTop;
    indices.push_back(IndexCenter);
    indices.push_back(IndexTemp1);
    indices.push_back(IndexTemp2);

    // second right (only if top neighbor has same LOD as core)
    if (lodTop == lodCore)
    {
        IndexTemp1 = IndexTemp2;
        IndexTemp2 += StepTop;
        indices.push_back(IndexCenter);
        indices.push_back(IndexTemp1);
        indices.push_back(IndexTemp2);
    }

    // first down (right edge of bottom sector)
    IndexTemp1 = IndexTemp2;
    IndexTemp2 -= StepRight * stride;
    indices.push_back(IndexCenter);
    indices.push_back(IndexTemp1);
    indices.push_back(IndexTemp2);

    // second down (only if right neighbor has same LOD as core)
    if (lodRight == lodCore)
    {
        IndexTemp1 = IndexTemp2;
        IndexTemp2 -= StepRight * stride;
        indices.push_back(IndexCenter);
        indices.push_back(IndexTemp1);
        indices.push_back(IndexTemp2);
    }

    // first left (bottom edge of left sector)
    IndexTemp1 = IndexTemp2;
    IndexTemp2 -= StepBottom;
    indices.push_back(IndexCenter);
    indices.push_back(IndexTemp1);
    indices.push_back(IndexTemp2);

    // second left (only if bottom neighbor has same LOD as core)
    if (lodBottom == lodCore)
    {
        IndexTemp1 = IndexTemp2;
        IndexTemp2 -= StepBottom;
        indices.push_back(IndexCenter);
        indices.push_back(IndexTemp1);
        indices.push_back(IndexTemp2);
    }
}

//=============================================================================
// Generate LOD index templates and build the static master index buffer.
//
// Builds one large GPU index buffer containing all 16 permutations for
// all LOD levels. The buffer is created once and NEVER changes.
// Each permutation's {start, count} is recorded in mLODPermutationInfo
// so UpdateLOD can reference them by index offset + baseVertex.
//
// All indices use the global row stride (gridSize) for row offsets,
// so that baseVertex = patchStartZ * gridSize + patchStartX correctly
// maps them to the right vertices in the global vertex buffer.
//=============================================================================

void GeoMipTerrain::GenerateLODIndexTemplates()
{
    uint32_t patchStride = mPatchSize - 1;

    // Calculate max LOD: log2(patchStride) - 1
    mMaxLOD = 0;
    uint32_t temp = patchStride;
    while (temp > 1)
    {
        temp >>= 1;
        mMaxLOD++;
    }
    // ogldev convention: coarsest LOD still has at least 2x2 cells per patch
    if (mMaxLOD > 0) mMaxLOD--;

    // Build patch metadata
    float step     = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    mPatches.resize(mPatchesPerSide * mPatchesPerSide);
    mPatchLods.resize(mPatchesPerSide * mPatchesPerSide);

    for (uint32_t pz = 0; pz < mPatchesPerSide; ++pz)
    {
        for (uint32_t px = 0; px < mPatchesPerSide; ++px)
        {
            uint32_t patchIdx = pz * mPatchesPerSide + px;
            PatchInfo& patch  = mPatches[patchIdx];
            patch.startX = px * patchStride;
            patch.startZ = pz * patchStride;

            // Center in world space
            float centerGridX = (float)(patch.startX + patchStride * 0.5f);
            float centerGridZ = (float)(patch.startZ + patchStride * 0.5f);
            patch.centerX = -halfSize + centerGridX * step;
            patch.centerZ = -halfSize + centerGridZ * step;
        }
    }

    // Build the master index buffer: all 16 permutations for all LOD levels
    // Indices are LOCAL within the patch (0-based, relative to patch origin)
    mLODPermutationInfo.resize(mMaxLOD + 1);

    std::vector<uint32_t> masterIndices;

    for (uint32_t lod = 0; lod <= mMaxLOD; ++lod)
    {
        uint32_t FanStep = PowInt(2, lod + 1);   // lod=0 -> 2, lod=1 -> 4, lod=2 -> 8
        uint32_t EndPos  = mPatchSize - 1 - FanStep;

        for (uint32_t l = 0; l < 2; ++l)
        {
            for (uint32_t r = 0; r < 2; ++r)
            {
                for (uint32_t t = 0; t < 2; ++t)
                {
                    for (uint32_t b = 0; b < 2; ++b)
                    {
                        uint32_t lodLeft   = lod + l;
                        uint32_t lodRight  = lod + r;
                        uint32_t lodTop    = lod + t;
                        uint32_t lodBottom = lod + b;

                        // Record where this permutation starts in the master buffer
                        uint32_t startIdx = (uint32_t)masterIndices.size();
                        mLODPermutationInfo[lod].info[l][r][t][b].start = startIdx;

                        // Generate indices for this permutation
                        for (uint32_t z = 0; z <= EndPos; z += FanStep)
                        {
                            for (uint32_t x = 0; x <= EndPos; x += FanStep)
                            {
                                // Edge detection: only boundary cells use neighbor LOD
                                uint32_t lLeft   = (x == 0)      ? lodLeft   : lod;
                                uint32_t lRight  = (x == EndPos) ? lodRight  : lod;
                                uint32_t lBottom = (z == 0)      ? lodBottom : lod;
                                uint32_t lTop    = (z == EndPos) ? lodTop    : lod;

                                CreateTriangleFan(masterIndices, lod, lLeft, lRight, lTop, lBottom, x, z, mGridSize);
                            }
                        }

                        // Record count
                        uint32_t endIdx = (uint32_t)masterIndices.size();
                        mLODPermutationInfo[lod].info[l][r][t][b].count = endIdx - startIdx;
                    }
                }
            }
        }

        LOG_INFO("GeoMipTerrain: LOD %u - FanStep=%u, 16 permutations generated", lod, FanStep);
    }

    // Create the static GPU index buffer (never changes after this)
    mMesh->SetIndices(masterIndices.data(), (uint32_t)masterIndices.size());
    auto indexBuffer = GetRenderDevice()->CreateIndexBufferWithBytes(
        masterIndices.data(),
        (uint32_t)masterIndices.size() * sizeof(uint32_t),
        IndexType_UInt);

    // Replace the index buffer created by SetUpBuffer with our master buffer
    // We access the mesh's private member through the setter pattern
    // Since Mesh doesn't have SetIndexBuffer, we use the fact that
    // UpdateIndices creates a new index buffer from mIndices.
    // But we already called SetIndices + SetUpBuffer, so we need to
    // recreate the index buffer with the master data.
    // The simplest approach: use UpdateIndices to create the GPU buffer.
    mMesh->UpdateIndices(masterIndices.data(), masterIndices.size());

    LOG_INFO("GeoMipTerrain: Master index buffer created, %u total indices", (uint32_t)masterIndices.size());
}

//=============================================================================
// DistanceToLOD - map a distance value to a LOD level
//=============================================================================

uint32_t GeoMipTerrain::DistanceToLOD(float distance) const
{
    for (uint32_t lod = 0; lod <= mMaxLOD; ++lod)
    {
        if (distance < mLODDistances[lod])
        {
            return lod;
        }
    }
    return mMaxLOD;
}

//=============================================================================
// UpdateLODMapPass1 - compute Core LOD for each patch from camera distance
//=============================================================================

void GeoMipTerrain::UpdateLODMapPass1(const Vector3f& cameraPos)
{
    for (uint32_t pz = 0; pz < mPatchesPerSide; ++pz)
    {
        for (uint32_t px = 0; px < mPatchesPerSide; ++px)
        {
            uint32_t patchIdx = pz * mPatchesPerSide + px;
            const PatchInfo& patch = mPatches[patchIdx];

            float dx = cameraPos.x - patch.centerX;
            float dz = cameraPos.z - patch.centerZ;
            float distance = sqrtf(dx * dx + dz * dz);

            mPatchLods[patchIdx].core = DistanceToLOD(distance);
        }
    }
}

//=============================================================================
// UpdateLODMapPass2 - compare each patch's Core LOD with its neighbors.
// If a neighbor has a strictly higher (coarser) LOD, set the flag to 1
// indicating crack fixing is needed on that edge.
//=============================================================================

void GeoMipTerrain::UpdateLODMapPass2()
{
    for (uint32_t pz = 0; pz < mPatchesPerSide; ++pz)
    {
        for (uint32_t px = 0; px < mPatchesPerSide; ++px)
        {
            uint32_t patchIdx = pz * mPatchesPerSide + px;
            uint32_t coreLod  = mPatchLods[patchIdx].core;

            // Left neighbor
            if (px > 0)
            {
                uint32_t leftCore = mPatchLods[pz * mPatchesPerSide + (px - 1)].core;
                mPatchLods[patchIdx].left = (leftCore > coreLod) ? 1 : 0;
            }
            else
            {
                mPatchLods[patchIdx].left = 0;
            }

            // Right neighbor
            if (px < mPatchesPerSide - 1)
            {
                uint32_t rightCore = mPatchLods[pz * mPatchesPerSide + (px + 1)].core;
                mPatchLods[patchIdx].right = (rightCore > coreLod) ? 1 : 0;
            }
            else
            {
                mPatchLods[patchIdx].right = 0;
            }

            // Bottom neighbor (z-1)
            if (pz > 0)
            {
                uint32_t bottomCore = mPatchLods[(pz - 1) * mPatchesPerSide + px].core;
                mPatchLods[patchIdx].bottom = (bottomCore > coreLod) ? 1 : 0;
            }
            else
            {
                mPatchLods[patchIdx].bottom = 0;
            }

            // Top neighbor (z+1)
            if (pz < mPatchesPerSide - 1)
            {
                uint32_t topCore = mPatchLods[(pz + 1) * mPatchesPerSide + px].core;
                mPatchLods[patchIdx].top = (topCore > coreLod) ? 1 : 0;
            }
            else
            {
                mPatchLods[patchIdx].top = 0;
            }
        }
    }
}

//=============================================================================
// UpdateLOD - update per-patch SubMeshInfo entries based on camera position.
//
// Uses the two-pass LOD algorithm to compute per-patch Core LOD and
// neighbor flags, then creates one SubMeshInfo per patch referencing
// the appropriate range in the static master index buffer.
//
// The baseVertex parameter offsets local patch indices (0-based) to
// the correct global vertex position: baseVertex = startZ * gridSize + startX
//
// This is much cheaper than the old approach: no per-frame index buffer
// rebuild or GPU upload. Only the SubMeshInfo list is updated (CPU-only).
//=============================================================================

void GeoMipTerrain::UpdateLOD(const Vector3f& cameraPos)
{
    if (!mMesh || mPatches.empty()) return;

    // Two-pass LOD update
    UpdateLODMapPass1(cameraPos);
    UpdateLODMapPass2();

    // Rebuild SubMeshInfo list: one per patch
    mMesh->ClearSubMeshInfos();

    uint32_t patchCount = (uint32_t)mPatches.size();

    for (uint32_t i = 0; i < patchCount; ++i)
    {
        uint32_t core = mPatchLods[i].core;
        uint32_t l    = mPatchLods[i].left;
        uint32_t r    = mPatchLods[i].right;
        uint32_t t    = mPatchLods[i].top;
        uint32_t b    = mPatchLods[i].bottom;

        const PatchInfo& patch = mPatches[i];
        const SinglePermutationInfo& permInfo = mLODPermutationInfo[core].info[l][r][t][b];

        SubMeshInfo subMeshInfo;
        subMeshInfo.firstIndex  = permInfo.start;
        subMeshInfo.indexCount  = permInfo.count;
        subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
        subMeshInfo.vertexCount = mPatchSize * mPatchSize;
        // baseVertex offsets local patch indices to global vertex positions
        subMeshInfo.baseVertex  = (int32_t)(patch.startZ * mGridSize + patch.startX);

        mMesh->AddSubMeshInfo(subMeshInfo);
    }
}

//=============================================================================
// SelectLOD - simple per-patch LOD selection (fallback, not used with crack fixing)
//=============================================================================

uint32_t GeoMipTerrain::SelectLOD(const Vector3f& cameraPos, uint32_t patchIdx) const
{
    const PatchInfo& patch = mPatches[patchIdx];

    float dx = cameraPos.x - patch.centerX;
    float dz = cameraPos.z - patch.centerZ;
    float distSq = dx * dx + dz * dz;

    for (uint32_t lod = 0; lod < mLODDistances.size() && lod <= mMaxLOD; ++lod)
    {
        float threshold = mLODDistances[lod];
        if (distSq < threshold * threshold)
        {
            return lod;
        }
    }

    return mMaxLOD;
}

//=============================================================================
// Set LOD distance thresholds
//=============================================================================

void GeoMipTerrain::SetLODDistances(const std::vector<float>& distances)
{
    mLODDistances = distances;
    // Ensure we have at least maxLOD + 1 entries
    while (mLODDistances.size() <= mMaxLOD)
    {
        float lastDist = mLODDistances.empty() ? 100.0f : mLODDistances.back() * 2.0f;
        mLODDistances.push_back(lastDist);
    }
}

//=============================================================================
// Get height at world position (bilinear interpolation)
//=============================================================================

float GeoMipTerrain::GetHeight(float worldX, float worldZ) const
{
    if (mHeightMap.empty()) return 0.0f;

    float halfSize = mWorldSize * 0.5f;
    float step     = mWorldSize / (float)(mGridSize - 1);

    float fx = (worldX + halfSize) / step;
    float fz = (worldZ + halfSize) / step;

    fx = std::max(0.0f, std::min(fx, (float)(mGridSize - 1)));
    fz = std::max(0.0f, std::min(fz, (float)(mGridSize - 1)));

    uint32_t x0 = (uint32_t)fx;
    uint32_t z0 = (uint32_t)fz;
    uint32_t x1 = std::min(x0 + 1, mGridSize - 1);
    uint32_t z1 = std::min(z0 + 1, mGridSize - 1);

    float dx = fx - x0;
    float dz = fz - z0;

    float h00 = mHeightMap[z0 * mGridSize + x0];
    float h10 = mHeightMap[z0 * mGridSize + x1];
    float h01 = mHeightMap[z1 * mGridSize + x0];
    float h11 = mHeightMap[z1 * mGridSize + x1];

    return h00 * (1.0f - dx) * (1.0f - dz)
         + h10 * dx * (1.0f - dz)
         + h01 * (1.0f - dx) * dz
         + h11 * dx * dz;
}

NS_RENDERSYSTEM_END
