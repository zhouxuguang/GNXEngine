//
//  GeoMipTerrain.cpp
//  GNXEngine
//
//  GeoMipMapping terrain with LOD support.
//  Generates a Mesh for use with the deferred rendering pipeline,
//  and provides per-patch LOD selection via UpdateLOD().
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

    // Set default LOD distances (doubling per level)
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

    // Placeholder indices (will be replaced by UpdateLOD)
    // Use full-detail indices as the initial state
    uint32_t indexCount = (mGridSize - 1) * (mGridSize - 1) * 6;
    std::vector<uint32_t> indices(indexCount);
    uint32_t iidx = 0;
    for (uint32_t iz = 0; iz < mGridSize - 1; ++iz)
    {
        for (uint32_t ix = 0; ix < mGridSize - 1; ++ix)
        {
            uint32_t v00 = iz * mGridSize + ix;
            uint32_t v10 = v00 + 1;
            uint32_t v01 = v00 + mGridSize;
            uint32_t v11 = v01 + 1;

            indices[iidx++] = v00;
            indices[iidx++] = v10;
            indices[iidx++] = v01;
            indices[iidx++] = v10;
            indices[iidx++] = v11;
            indices[iidx++] = v01;
        }
    }
    mMesh->SetIndices(indices.data(), indexCount);

    SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex  = 0;
    subMeshInfo.indexCount  = indexCount;
    subMeshInfo.vertexCount = vertexCount;
    subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
    mMesh->AddSubMeshInfo(subMeshInfo);

    mMesh->SetUpBuffer();
}

//=============================================================================
// Generate LOD index templates (local indices within a patch)
//=============================================================================

void GeoMipTerrain::GenerateLODIndexTemplates()
{
    uint32_t patchStride = mPatchSize - 1;

    // Calculate max LOD: log2(patchStride)
    mMaxLOD = 0;
    uint32_t temp = patchStride;
    while (temp > 1)
    {
        temp >>= 1;
        mMaxLOD++;
    }

    // Build patch metadata
    float step     = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    mPatches.resize(mPatchesPerSide * mPatchesPerSide);
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

    // Generate index templates for each LOD level
    // Template indices are LOCAL to the patch: localIdx = lz * patchSize + lx
    mLODIndexTemplates.resize(mMaxLOD + 1);
    mLODIndexCount.resize(mMaxLOD + 1);

    for (uint32_t lod = 0; lod <= mMaxLOD; ++lod)
    {
        uint32_t lodStep = 1u << lod;
        std::vector<uint32_t>& tmpl = mLODIndexTemplates[lod];

        for (uint32_t lz = 0; lz < patchStride; lz += lodStep)
        {
            for (uint32_t lx = 0; lx < patchStride; lx += lodStep)
            {
                uint32_t v00 = lz * mPatchSize + lx;
                uint32_t v10 = lz * mPatchSize + (lx + lodStep);
                uint32_t v01 = (lz + lodStep) * mPatchSize + lx;
                uint32_t v11 = (lz + lodStep) * mPatchSize + (lx + lodStep);

                tmpl.push_back(v00);
                tmpl.push_back(v10);
                tmpl.push_back(v01);
                tmpl.push_back(v10);
                tmpl.push_back(v11);
                tmpl.push_back(v01);
            }
        }

        mLODIndexCount[lod] = (uint32_t)tmpl.size();

        LOG_INFO("GeoMipTerrain: LOD %u - step=%u, indices/patch=%u",
                 lod, lodStep, mLODIndexCount[lod]);
    }
}

//=============================================================================
// UpdateLOD - rebuild the Mesh index buffer based on camera position
//=============================================================================

void GeoMipTerrain::UpdateLOD(const Vector3f& cameraPos)
{
    if (!mMesh || mPatches.empty()) return;

    uint32_t patchStride = mPatchSize - 1;
    uint32_t patchCount  = (uint32_t)mPatches.size();

    // Estimate total index count (use max possible for reserve)
    uint32_t maxTotalIndices = 0;
    for (uint32_t i = 0; i < patchCount; ++i)
    {
        maxTotalIndices += mLODIndexCount[0]; // LOD 0 has the most indices
    }
    std::vector<uint32_t> combinedIndices;
    combinedIndices.reserve(maxTotalIndices);

    // Build combined index buffer
    for (uint32_t i = 0; i < patchCount; ++i)
    {
        uint32_t lod = SelectLOD(cameraPos, i);
        const PatchInfo& patch       = mPatches[i];
        const std::vector<uint32_t>& tmpl = mLODIndexTemplates[lod];

        // Convert local indices to global indices
        uint32_t baseX = patch.startX;
        uint32_t baseZ = patch.startZ;

        for (uint32_t localIdx : tmpl)
        {
            uint32_t lx = localIdx % mPatchSize;
            uint32_t lz = localIdx / mPatchSize;
            uint32_t globalIdx = (baseZ + lz) * mGridSize + (baseX + lx);
            combinedIndices.push_back(globalIdx);
        }
    }

    // Update the mesh's index buffer
    mMesh->UpdateIndices(combinedIndices.data(), combinedIndices.size());
}

//=============================================================================
// LOD selection based on camera distance
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
