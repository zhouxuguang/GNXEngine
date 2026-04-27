//
//  QuadTreeTerrain.cpp
//  GNXEngine
//
//  Quadtree-based terrain with adaptive LOD.
//  No crack fixing for now.
//

#include "terrain/QuadTreeTerrain.h"
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
// Procedural height function (same as GeoMipTerrain)
//=============================================================================

float QuadTreeTerrain::ComputeHeight(float x, float z)
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

QuadTreeTerrain::QuadTreeTerrain() = default;
QuadTreeTerrain::~QuadTreeTerrain() = default;

//=============================================================================
// Factory: Create from heightmap image
//=============================================================================

QuadTreeTerrainPtr QuadTreeTerrain::CreateFromHeightMap(
    const char* heightmapPath,
    float worldSizeXZ,
    float heightScale,
    uint32_t maxLevel)
{
    using namespace imagecodec;

    // Load heightmap
    VImage heightmapImage;
    if (!ImageDecoder::DecodeFile(heightmapPath, &heightmapImage))
    {
        LOG_ERROR("QuadTreeTerrain: Failed to load heightmap: %s", heightmapPath);
        return nullptr;
    }

    if (heightmapImage.GetFormat() != FORMAT_GRAY8 && heightmapImage.GetFormat() != FORMAT_GRAY16)
    {
        LOG_ERROR("QuadTreeTerrain: Heightmap must be GRAY8 or GRAY16, got format %d",
                  heightmapImage.GetFormat());
        return nullptr;
    }

    uint32_t imageWidth  = heightmapImage.GetWidth();
    uint32_t imageHeight = heightmapImage.GetHeight();
    if (imageWidth != imageHeight)
    {
        LOG_WARN("QuadTreeTerrain: Heightmap is not square (%ux%u), using width",
                 imageWidth, imageHeight);
    }

    // Round grid size up to next 2^n + 1
    uint32_t n = 0;
    uint32_t cells = imageWidth - 1;
    while ((1u << n) < cells) ++n;
    uint32_t gridSize = (1u << n) + 1;

    auto terrain = QuadTreeTerrainPtr(new QuadTreeTerrain());
    terrain->mWorldSize     = worldSizeXZ;
    terrain->mHeightScale   = heightScale;
    terrain->mGridSize      = gridSize;
    terrain->mGridSizeCells = gridSize - 1;

    // Clamp maxLevel so minimum node size >= (kLeafVerticesPerSide - 1)
    uint32_t minNodeCells = kLeafVerticesPerSide - 1;  // 16
    uint32_t maxPossibleLevel = 0;
    uint32_t tmp = terrain->mGridSizeCells;
    while (tmp > minNodeCells) { tmp >>= 1; ++maxPossibleLevel; }
    terrain->mMaxLevel = std::min(maxLevel, maxPossibleLevel);

    LOG_INFO("QuadTreeTerrain: Heightmap %s (%ux%u), grid=%u, maxLevel=%u",
             heightmapPath, imageWidth, imageHeight, gridSize, terrain->mMaxLevel);

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

    // Compute normals via central differences
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

    // Compute tangents
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

    // Build quadtree
    terrain->BuildNode(&terrain->mRoot);

    // Pre-compute geometric errors for SSE LOD selection
    terrain->ComputeAllGeoErrors();

    // Build static index pool BEFORE first Update()
    // (Update→GenerateLeafMesh reads from mIndexPool, must be populated)
    terrain->BuildStaticIndexPool();

    // Initial update (now safe — pool is ready)
    terrain->Update(Vector3f(0, 0, 0));

    LOG_INFO("QuadTreeTerrain: Created from heightmap, grid=%u, maxLevel=%u",
             gridSize, terrain->mMaxLevel);

    return terrain;
}

//=============================================================================
// Factory: Create from procedural noise
//=============================================================================

QuadTreeTerrainPtr QuadTreeTerrain::Create(
    uint32_t gridSize,
    float worldSizeXZ,
    float heightScale,
    uint32_t maxLevel)
{
    // Round up gridSize to 2^n + 1
    uint32_t n = 0;
    uint32_t cells = gridSize - 1;
    while ((1u << n) < cells) ++n;
    uint32_t adjustedGridSize = (1u << n) + 1;

    auto terrain = QuadTreeTerrainPtr(new QuadTreeTerrain());
    terrain->mWorldSize     = worldSizeXZ;
    terrain->mHeightScale   = heightScale;
    terrain->mGridSize      = adjustedGridSize;
    terrain->mGridSizeCells = adjustedGridSize - 1;

    // Clamp maxLevel
    uint32_t minNodeCells = kLeafVerticesPerSide - 1;
    uint32_t maxPossibleLevel = 0;
    uint32_t tmp = terrain->mGridSizeCells;
    while (tmp > minNodeCells) { tmp >>= 1; ++maxPossibleLevel; }
    terrain->mMaxLevel = std::min(maxLevel, maxPossibleLevel);

    uint32_t vertexCount = adjustedGridSize * adjustedGridSize;
    float step           = worldSizeXZ / (float)(adjustedGridSize - 1);
    float halfSize       = worldSizeXZ * 0.5f;

    terrain->mHeightMap.resize(vertexCount);

    std::vector<Vector3f> positions(vertexCount);
    std::vector<Vector3f> normals(vertexCount);
    std::vector<Vector4f> tangents(vertexCount);
    std::vector<Vector2f> uvs(vertexCount);

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

    // Build quadtree
    terrain->BuildNode(&terrain->mRoot);

    // Pre-compute geometric errors for SSE LOD selection
    terrain->ComputeAllGeoErrors();

    // Build static index pool BEFORE first Update()
    terrain->BuildStaticIndexPool();

    // Initial update (now safe — pool is ready)
    terrain->Update(Vector3f(0, 0, 0));

    LOG_INFO("QuadTreeTerrain: Created procedural, grid=%u, maxLevel=%u",
             adjustedGridSize, terrain->mMaxLevel);

    return terrain;
}

//=============================================================================
// Initialize vertex data and create GPU vertex buffer
//=============================================================================

void QuadTreeTerrain::InitVertexData(
    const std::vector<Vector3f>& positions,
    const std::vector<Vector3f>& normals,
    const std::vector<Vector4f>& tangents,
    const std::vector<Vector2f>& uvs)
{
    uint32_t vertexCount = mGridSize * mGridSize;

    mMesh = std::make_shared<Mesh>();

    VertexData& vertexData = mMesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f);
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

    mMesh->SetUpBuffer();
}

//=============================================================================
// Build quadtree root node
//=============================================================================

void QuadTreeTerrain::BuildNode(Node* node)
{
    node->x     = 0;
    node->z     = 0;
    node->size  = mGridSizeCells;
    node->level = mMaxLevel;  // root is the coarsest LOD
    ComputeNodeBounds(node);
}

//=============================================================================
// Compute world-space AABB for a node
//=============================================================================

void QuadTreeTerrain::ComputeNodeBounds(Node* node)
{
    float step     = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    // Grid range [x, x+size] cells → vertex range [x, x+size]
    float worldMinX = -halfSize + (float)node->x * step;
    float worldMinZ = -halfSize + (float)node->z * step;
    float worldMaxX = -halfSize + (float)(node->x + node->size) * step;
    float worldMaxZ = -halfSize + (float)(node->z + node->size) * step;

    // Find min/max height in this node's region
    float minY =  std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (uint32_t iz = node->z; iz <= node->z + node->size && iz < mGridSize; ++iz)
    {
        for (uint32_t ix = node->x; ix <= node->x + node->size && ix < mGridSize; ++ix)
        {
            float h = mHeightMap[iz * mGridSize + ix];
            minY = std::min(minY, h);
            maxY = std::max(maxY, h);
        }
    }

    node->bounds.minimum = Vector3f(worldMinX, minY, worldMinZ);
    node->bounds.maximum = Vector3f(worldMaxX, maxY, worldMaxZ);
    node->bounds.center  = (node->bounds.minimum + node->bounds.maximum) * 0.5f;
}

//=============================================================================
// ShouldSubdivide - determine if a node should be split into 4 children
//=============================================================================

bool QuadTreeTerrain::ShouldSubdivide(const Node& node, const Vector3f& cameraPos) const
{
    // Cannot subdivide beyond finest level
    if (node.level == 0) return false;

    // No geometric error means this LOD perfectly represents the terrain
    if (node.maxGeoError <= 0.0f) return false;

    // SSE-based test when camera parameters are available
    if (mTanHalfFovY > 0.0f && mScreenHeight > 0.0f)
    {
        // Distance from camera to closest point on node's AABB
        float dx = std::max(0.0f, std::max(node.bounds.minimum.x - cameraPos.x,
                                            cameraPos.x - node.bounds.maximum.x));
        float dy = std::max(0.0f, std::max(node.bounds.minimum.y - cameraPos.y,
                                            cameraPos.y - node.bounds.maximum.y));
        float dz = std::max(0.0f, std::max(node.bounds.minimum.z - cameraPos.z,
                                            cameraPos.z - node.bounds.maximum.z));
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
        distance = std::max(distance, 0.001f);  // avoid division by zero

        // Screen-space error: screenError = (geoError * tanHalfFov * screenHeight) / distance
        // Subdivide when screenError exceeds threshold
        float screenError = (node.maxGeoError * mTanHalfFovY * mScreenHeight) / distance;
        return screenError > mSSEThreshold;
    }

    // Fallback: distance-based test (when camera params not provided)
    float nodeWorldSize = (float)node.size * (mWorldSize / (float)(mGridSize - 1));
    float threshold = nodeWorldSize * mLODDistanceFactor;

    Vector3f nodeCenter = node.bounds.center;
    float cdx = cameraPos.x - nodeCenter.x;
    float cdy = cameraPos.y - nodeCenter.y;
    float cdz = cameraPos.z - nodeCenter.z;
    float distance = sqrtf(cdx * cdx + cdy * cdy + cdz * cdz);

    return distance < threshold;
}

//=============================================================================
// Subdivide - create 4 children for a node
//=============================================================================

void QuadTreeTerrain::Subdivide(Node* node)
{
    uint32_t halfSize = node->size / 2;
    uint32_t childLevel = node->level - 1;  // go finer: 5→4→3→2→1→0

    // NW (top-left)
    node->children[0] = std::make_unique<Node>();
    node->children[0]->x     = node->x;
    node->children[0]->z     = node->z;
    node->children[0]->size  = halfSize;
    node->children[0]->level = childLevel;
    node->children[0]->parent = node;
    ComputeNodeBounds(node->children[0].get());
    node->children[0]->maxGeoError = GetCachedGeoError(childLevel, node->x, node->z);

    // NE (top-right)
    node->children[1] = std::make_unique<Node>();
    node->children[1]->x     = node->x + halfSize;
    node->children[1]->z     = node->z;
    node->children[1]->size  = halfSize;
    node->children[1]->level = childLevel;
    node->children[1]->parent = node;
    ComputeNodeBounds(node->children[1].get());
    node->children[1]->maxGeoError = GetCachedGeoError(childLevel, node->x + halfSize, node->z);

    // SW (bottom-left)
    node->children[2] = std::make_unique<Node>();
    node->children[2]->x     = node->x;
    node->children[2]->z     = node->z + halfSize;
    node->children[2]->size  = halfSize;
    node->children[2]->level = childLevel;
    node->children[2]->parent = node;
    ComputeNodeBounds(node->children[2].get());
    node->children[2]->maxGeoError = GetCachedGeoError(childLevel, node->x, node->z + halfSize);

    // SE (bottom-right)
    node->children[3] = std::make_unique<Node>();
    node->children[3]->x     = node->x + halfSize;
    node->children[3]->z     = node->z + halfSize;
    node->children[3]->size  = halfSize;
    node->children[3]->level = childLevel;
    node->children[3]->parent = node;
    ComputeNodeBounds(node->children[3].get());
    node->children[3]->maxGeoError = GetCachedGeoError(childLevel, node->x + halfSize, node->z + halfSize);
}

//=============================================================================
// UpdateNode - recursively update quadtree based on camera position
//=============================================================================

void QuadTreeTerrain::UpdateNode(Node* node, const Vector3f& cameraPos)
{
    if (ShouldSubdivide(*node, cameraPos))
    {
        // Need subdivision
        if (node->IsLeaf())
        {
            Subdivide(node);
        }

        // Recurse into children
        for (int i = 0; i < 4; ++i)
        {
            UpdateNode(node->children[i].get(), cameraPos);
        }
    }
    else
    {
        // No subdivision needed - collapse children if any
        for (int i = 0; i < 4; ++i)
        {
            node->children[i].reset();
        }
    }
}

//=============================================================================
// CollectLeaves - gather all leaf nodes into mLeafNodes
//=============================================================================

void QuadTreeTerrain::CollectLeaves(Node* node)
{
    if (node->IsLeaf())
    {
        mLeafNodes.push_back(node);
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (node->children[i])
        {
            CollectLeaves(node->children[i].get());
        }
    }
}

//=============================================================================
// GenerateLeafMesh - build SubMeshInfo list from current leaf nodes.
//
// Looks up pre-computed index buffers from the static pool (built by
// BuildStaticIndexPool). No index generation, no GPU upload — O(1) per leaf.
//
// All indices in the static pool are LEAF-LOCAL. We set baseVertex per
// leaf to offset them into the global terrain vertex buffer.
//=============================================================================

void QuadTreeTerrain::GenerateLeafMesh()
{
    // Clear previous frame's SubMeshInfos
    mMesh->ClearSubMeshInfos();
    mLeafBounds.clear();
    mLeafBounds.reserve(mLeafNodes.size());

    for (size_t idx = 0; idx < mLeafNodes.size(); ++idx)
    {
        Node* leaf = mLeafNodes[idx];
        const LeafNeighborInfo& nbrInfo = mLeafNeighborInfo[idx];

        // Compute stride for this leaf's LOD level
        uint32_t cellsPerSide = kLeafVerticesPerSide - 1;  // 16
        uint32_t stride = leaf->size / cellsPerSide;

        // Map stride → level index into the static pool
        uint32_t strideLevel = GetStrideLevel(stride);

        // Encode 4 neighbor coarser flags as a permutation index [0..15]
        uint32_t perm = (nbrInfo.leftCoarser   ? 1u : 0u)
                      | (nbrInfo.rightCoarser  ? 2u : 0u)
                      | (nbrInfo.topCoarser    ? 4u : 0u)
                      | (nbrInfo.bottomCoarser ? 8u : 0u);

        // Look up pre-computed indices from the static pool
        const IndexPoolEntry& entry = mIndexPool[strideLevel][perm];

        SubMeshInfo subMeshInfo;
        subMeshInfo.firstIndex = entry.start;
        subMeshInfo.indexCount = entry.count;
        subMeshInfo.topology   = PrimitiveMode_TRIANGLES;
        subMeshInfo.vertexCount = mGridSize * mGridSize;
        // baseVertex offsets leaf-local indices to absolute positions in global VB
        subMeshInfo.baseVertex = leaf->z * mGridSize + leaf->x;
        mMesh->AddSubMeshInfo(subMeshInfo);
        mLeafBounds.push_back(leaf->bounds);
    }

    // NO UpdateIndices call — buffer is static, built once at init time
}

//=============================================================================
// CreateTriangleFanLocal - generate triangles for a single fan-cell with crack fixing.
//
// Outputs SEMI-LOCAL indices: uses GLOBAL row stride (mGridSize) but LEAF-LOCAL
// coordinates. This allows baseVertex to correctly offset into the global VB:
//
//   finalIndex = poolIndex + baseVertex
//             = (local_z * mGridSize + local_x) + (leaf->z * mGridSize + leaf->x)
//             = (leaf->z + local_z) * mGridSize + (leaf->x + local_x)  ✓
//
// (fcx, fcz): fan-cell origin in LEAF-LOCAL grid coordinates
// globalGridSize: mGridSize (global terrain vertices per side)
//=============================================================================

void QuadTreeTerrain::CreateTriangleFanLocal(
    std::vector<uint32_t>& indices,
    uint32_t fcx, uint32_t fcz,      // fan-cell origin (leaf-local grid coords)
    uint32_t stride,                 // half of fan-cell size; center = origin + stride
    uint32_t globalGridSize,         // MUST be mGridSize — global row stride
    bool leftCoarser,               // left  neighbor (-X) is coarser?
    bool rightCoarser,              // right neighbor (+X) is coarser?
    bool topCoarser,                // top   neighbor (-Z) is coarser?
    bool bottomCoarser)             // bottom neighbor (+Z) is coarser?
{
    // Effective step per direction: double stride when neighbor is coarser.
    uint32_t sLeft   = leftCoarser   ? 2 * stride : stride;
    uint32_t sRight  = rightCoarser  ? 2 * stride : stride;
    uint32_t sTop    = topCoarser    ? 2 * stride : stride;
    uint32_t sBottom = bottomCoarser ? 2 * stride : stride;

    // Fan center in semi-local coords (global row stride, leaf-local offsets)
    uint32_t idxCenter = (fcz + stride) * globalGridSize + (fcx + stride);

    // ---- Sector 1: LEFT edge (from TL to BL), traversing downward ----
    uint32_t t1 = fcz * globalGridSize + fcx;
    uint32_t t2 = (fcz + sLeft) * globalGridSize + fcx;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!leftCoarser)
    {
        t1 = t2; t2 += sLeft * globalGridSize;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- Sector 2: BOTTOM edge (from BL to BR), traversing rightward ----
    // Continue from Sector 1's end
    t1 = t2; t2 = t1 + sBottom;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!bottomCoarser)
    {
        t1 = t2; t2 += sBottom;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- Sector 3: RIGHT edge (from BR to TR), traversing upward ----
    // Continue from Sector 2's end
    t1 = t2; t2 = t1 - sRight * globalGridSize;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!rightCoarser)
    {
        t1 = t2; t2 -= sRight * globalGridSize;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- Sector 4: TOP edge (from TR to TL), traversing leftward ----
    // Continue from Sector 3's end
    t1 = t2; t2 = t1 - sTop;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!topCoarser)
    {
        t1 = t2; t2 -= sTop;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }
}

//=============================================================================
// GetStrideLevel - map stride value to level index (log2).
// stride must be a power of 2: returns 0 for stride=1, 1 for stride=2, etc.
//=============================================================================

uint32_t QuadTreeTerrain::GetStrideLevel(uint32_t stride) const
{
    uint32_t level = 0;
    while (stride > 1) { stride >>= 1; ++level; }
    return level;
}

//=============================================================================
// ComputeNodeGeoError - compute max geometric error when a node is rendered
// at its LOD level (i.e., with kLeafVerticesPerSide vertices per side).
//
// The geometric error is the maximum |actual_height - bilinear_interpolated_height|
// over all grid vertices within the node that are NOT on the rendered vertex grid.
// When the node size equals kLeafVerticesPerSide-1, every vertex is rendered
// and the error is zero.
//=============================================================================

float QuadTreeTerrain::ComputeNodeGeoError(uint32_t x, uint32_t z, uint32_t size) const
{
    uint32_t cellsPerSide = kLeafVerticesPerSide - 1;  // 16
    if (size <= cellsPerSide) return 0.0f;  // finest detail, no error

    uint32_t stride = size / cellsPerSide;
    float maxError = 0.0f;

    for (uint32_t gz = z; gz <= z + size && gz < mGridSize; ++gz)
    {
        for (uint32_t gx = x; gx <= x + size && gx < mGridSize; ++gx)
        {
            // Skip rendered vertices (they lie exactly on the sampled grid)
            uint32_t localX = gx - x;
            uint32_t localZ = gz - z;
            if (localX % stride == 0 && localZ % stride == 0) continue;

            // Find which rendered grid cell this vertex falls in
            uint32_t cellX = localX / stride;
            uint32_t cellZ = localZ / stride;

            // Clamp to valid cell range
            cellX = std::min(cellX, cellsPerSide - 1);
            cellZ = std::min(cellZ, cellsPerSide - 1);

            // 4 corners of the rendered grid cell (global coords)
            uint32_t x0 = x + cellX * stride;
            uint32_t z0 = z + cellZ * stride;
            uint32_t x1 = std::min(x0 + stride, mGridSize - 1);
            uint32_t z1 = std::min(z0 + stride, mGridSize - 1);

            float h00 = mHeightMap[z0 * mGridSize + x0];
            float h10 = mHeightMap[z0 * mGridSize + x1];
            float h01 = mHeightMap[z1 * mGridSize + x0];
            float h11 = mHeightMap[z1 * mGridSize + x1];

            // Bilinear interpolation weights
            float tx = (x1 > x0) ? (float)(gx - x0) / (float)(x1 - x0) : 0.0f;
            float tz = (z1 > z0) ? (float)(gz - z0) / (float)(z1 - z0) : 0.0f;

            float interp = h00 * (1.0f - tx) * (1.0f - tz)
                         + h10 * tx * (1.0f - tz)
                         + h01 * (1.0f - tx) * tz
                         + h11 * tx * tz;

            float actual = mHeightMap[gz * mGridSize + gx];
            float error = fabsf(actual - interp);
            maxError = std::max(maxError, error);
        }
    }

    return maxError;
}

//=============================================================================
// ComputeAllGeoErrors - pre-compute geometric error for every possible node
// at every level. Called once at initialization.
//=============================================================================

void QuadTreeTerrain::ComputeAllGeoErrors()
{
    mGeoErrorCache.resize(mMaxLevel + 1);

    for (uint32_t level = 0; level <= mMaxLevel; ++level)
    {
        uint32_t nodeSize = mGridSizeCells >> level;
        uint32_t nodesPerSide = 1u << level;
        mGeoErrorCache[level].resize(nodesPerSide * nodesPerSide);

        for (uint32_t j = 0; j < nodesPerSide; ++j)
        {
            for (uint32_t i = 0; i < nodesPerSide; ++i)
            {
                uint32_t nx = i * nodeSize;
                uint32_t nz = j * nodeSize;
                mGeoErrorCache[level][j * nodesPerSide + i] =
                    ComputeNodeGeoError(nx, nz, nodeSize);
            }
        }
    }

    // Set root node's geoError (root level = mMaxLevel)
    mRoot.maxGeoError = GetCachedGeoError(mRoot.level, mRoot.x, mRoot.z);

    LOG_INFO("QuadTreeTerrain: GeoError cache computed for %u levels", mMaxLevel + 1);
}

//=============================================================================
// GetCachedGeoError - look up pre-computed geometric error from cache
//=============================================================================

float QuadTreeTerrain::GetCachedGeoError(uint32_t level, uint32_t x, uint32_t z) const
{
    // Convert LOD level to depth index: depth=0 is root (coarsest), depth=mMaxLevel is finest
    uint32_t depth = mMaxLevel - level;
    if (depth >= mGeoErrorCache.size()) return 0.0f;

    uint32_t nodeSize = mGridSizeCells >> depth;
    uint32_t nodesPerSide = 1u << depth;
    uint32_t i = x / nodeSize;
    uint32_t j = z / nodeSize;

    if (i >= nodesPerSide || j >= nodesPerSide) return 0.0f;

    return mGeoErrorCache[depth][j * nodesPerSide + i];
}

//=============================================================================
// BuildStaticIndexPool - pre-compute all index permutations for static IB.
//
// For each possible stride level (1, 2, 4, ..., maxStride), generates all 16
// neighbor-coarser permutations. Each entry contains the complete index buffer
// for one leaf configuration (64 fan-cells × 4~8 triangles each).
//
// All indices are LEAF-LOCAL. At runtime, GenerateLeafMesh() looks up the
// correct entry and sets baseVertex to offset into the global VB.
//
// Called once during terrain creation. After this, UpdateIndices() is never
// called again — zero per-frame GPU upload.
//=============================================================================

void QuadTreeTerrain::BuildStaticIndexPool()
{
    uint32_t minNodeCells = kLeafVerticesPerSide - 1;  // 16
    uint32_t maxStride = mGridSizeCells / minNodeCells;

    // Compute max stride level (log2 of maxStride)
    uint32_t maxStrideLevel = 0;
    { uint32_t tmp = maxStride; while (tmp > 1) { tmp >>= 1; ++maxStrideLevel; } }

    uint32_t numLevels = maxStrideLevel + 1;
    mIndexPool.resize(numLevels);
    for (auto& v : mIndexPool) v.resize(16);

    mMasterIndices.clear();
    mMasterIndices.reserve(numLevels * 16 * 1200);  // ~1200 indices per entry (estimate)

    uint32_t fanCellsPerSide = minNodeCells / 2;  // always 8

    for (uint32_t level = 0; level <= maxStrideLevel; ++level)
    {
        uint32_t stride = 1u << level;
        uint32_t leafSizeCells = stride * minNodeCells;  // 16 << level

        for (uint32_t perm = 0; perm < 16; ++perm)
        {
            bool leftC   = (perm & 1)  != 0;
            bool rightC  = (perm & 2)  != 0;
            bool topC    = (perm & 4)  != 0;
            bool bottomC = (perm & 8)  != 0;

            uint32_t startIdx = (uint32_t)mMasterIndices.size();

            // Generate all 64 fan-cells for this leaf configuration
            for (uint32_t fj = 0; fj < fanCellsPerSide; ++fj)
            {
                for (uint32_t fi = 0; fi < fanCellsPerSide; ++fi)
                {
                    // Fan-cell origin in LEAF-LOCAL coordinates
                    uint32_t fcx = fi * 2 * stride;
                    uint32_t fcz = fj * 2 * stride;

                    // Edge detection within the leaf's fan-cell grid
                    // fj=0 → fcz=0 → smallest Z → TOP edge (-Z)
                    // fj=max → largest Z → BOTTOM edge (+Z)
                    bool onLeftEdge   = (fi == 0);
                    bool onRightEdge  = (fi == fanCellsPerSide - 1);
                    bool onTopEdge    = (fj == 0);
                    bool onBottomEdge = (fj == fanCellsPerSide - 1);

                    bool lC = onLeftEdge   && leftC;
                    bool rC = onRightEdge  && rightC;
                    bool tC = onTopEdge    && topC;
                    bool bC = onBottomEdge && bottomC;

                    CreateTriangleFanLocal(mMasterIndices, fcx, fcz, stride,
                                           mGridSize, lC, rC, tC, bC);
                }
            }

            mIndexPool[level][perm].start = startIdx;
            mIndexPool[level][perm].count = (uint32_t)mMasterIndices.size() - startIdx;
        }

        LOG_INFO("QuadTreeTerrain: IndexPool level %u (stride=%u, leafCells=%u) built",
                 level, stride, leafSizeCells);
    }

    // Upload master index buffer to GPU — ONCE
    if (!mMasterIndices.empty())
    {
        mMesh->UpdateIndices(mMasterIndices.data(), (uint32_t)mMasterIndices.size());
    }

    LOG_INFO("QuadTreeTerrain: Static index pool complete: %u levels x 16 perms = %u entries, "
             "%u total indices (%.1f KB)",
             numLevels, numLevels * 16, (uint32_t)mMasterIndices.size(),
             (uint32_t)mMasterIndices.size() * sizeof(uint32_t) / 1024.0f);
}

//=============================================================================
// GetChildIndex - find which child index this node is within its parent
//=============================================================================

int QuadTreeTerrain::GetChildIndex(const Node* node)
{
    if (!node || !node->parent) return -1;
    for (int i = 0; i < 4; ++i)
    {
        if (node->parent->children[i].get() == node)
            return i;
    }
    return -1;
}

//=============================================================================
// GetMinNeighborLevel - find the minimum (finest) leaf level among all leaves
// adjacent to `node` on the specified edge.
//
// direction: 0=left(-X), 1=right(+X), 2=bottom(+Z), 3=top(-Z)
//
// Uses the standard quadtree neighbor-finding algorithm:
// 1. Walk up the tree to find an ancestor whose sibling is in the
//    desired direction.
// 2. Descend into that sibling, following children that share the edge,
//    to find the finest leaf.
//=============================================================================

uint32_t QuadTreeTerrain::GetMinNeighborLevel(const Node* node, int direction) const
{
    // Child layout:
    //   0=NW(x,    z,    h)  1=NE(x+h, z,    h)
    //   2=SW(x,    z+h,  h)  3=SE(x+h, z+h,  h)
    //
    // For each direction, some children have their neighbor as a sibling,
    // others must go up to the parent first.
    //
    // LEFT (-X):  child 1→sibling 0, child 3→sibling 2;  child 0,2→go up
    // RIGHT(+X):  child 0→sibling 1, child 2→sibling 3;  child 1,3→go up
    // BOTTOM(+Z): child 0→sibling 2, child 1→sibling 3;  child 2,3→go up
    // TOP (-Z):   child 2→sibling 0, child 3→sibling 1;  child 0,1→go up

    static const int kSibling[4][4] = {
        { -1,  0, -1,  2 },   // LEFT:  child 1→0, child 3→2
        {  1, -1,  3, -1 },   // RIGHT: child 0→1, child 2→3
        {  2,  3, -1, -1 },   // BOTTOM:child 0→2, child 1→3
        { -1, -1,  0,  1 },   // TOP:   child 2→0, child 3→1
    };

    // Children to explore when descending into the neighbor.
    // LEFT:  right-side  children [1,3] (they share the LEFT edge of neighbor)
    // RIGHT: left-side   children [0,2]
    // BOTTOM:top-side    children [0,1]
    // TOP:   bottom-side children [2,3]
    static const int kDescend[4][2] = {
        { 1, 3 },   // LEFT
        { 0, 2 },   // RIGHT
        { 0, 1 },   // BOTTOM
        { 2, 3 },   // TOP
    };

    // Walk up to find the neighbor at the same or coarser level
    const Node* current = node;
    const Node* neighbor = nullptr;

    while (current->parent)
    {
        int childIdx = GetChildIndex(current);
        int siblingIdx = kSibling[direction][childIdx];

        if (siblingIdx >= 0)
        {
            neighbor = current->parent->children[siblingIdx].get();
            break;
        }
        current = current->parent;
    }

    if (!neighbor)
    {
        // Terrain boundary — no neighbor in this direction
        return node->level;
    }

    // Descend into the neighbor to find the minimum leaf level (finest detail)
    // among all leaves sharing the edge with the original node.
    //
    // We must only follow children whose range on the perpendicular axis
    // overlaps the original node's range. Otherwise we might count
    // leaves that don't actually share the edge.

    uint32_t origMin, origMax;  // original node's range on the perpendicular axis
    if (direction <= 1)  // LEFT/RIGHT → perpendicular axis is Z
    {
        origMin = node->z;
        origMax = node->z + node->size;
    }
    else  // BOTTOM/TOP → perpendicular axis is X
    {
        origMin = node->x;
        origMax = node->x + node->size;
    }

    uint32_t minLevel = mMaxLevel;  // start with coarsest

    // DFS into the neighbor tree
    std::vector<const Node*> stack;
    stack.push_back(neighbor);

    while (!stack.empty())
    {
        const Node* n = stack.back();
        stack.pop_back();

        // Check if this node overlaps the original node's perpendicular range
        uint32_t nMin, nMax;
        if (direction <= 1)
        {
            nMin = n->z;
            nMax = n->z + n->size;
        }
        else
        {
            nMin = n->x;
            nMax = n->x + n->size;
        }

        if (nMin >= origMax || nMax <= origMin)
        {
            continue;  // no overlap, skip
        }

        if (n->IsLeaf())
        {
            minLevel = std::min(minLevel, n->level);
            continue;
        }

        for (int c : kDescend[direction])
        {
            if (n->children[c])
            {
                stack.push_back(n->children[c].get());
            }
        }
    }

    return minLevel;
}

//=============================================================================
// EnforceNeighborConstraint - ensure adjacent leaves differ by at most 1 level.
//
// After the camera-based subdivision, adjacent leaves can differ by 2+ levels
// (e.g., a level-1 leaf next to a level-3 leaf). This pass forcibly
// subdivides any leaf whose neighbor is more than 1 level finer, repeating
// until the constraint is satisfied.
//=============================================================================

void QuadTreeTerrain::EnforceNeighborConstraint()
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        mLeafNodes.clear();
        CollectLeaves(&mRoot);

        for (Node* leaf : mLeafNodes)
        {
            // Cannot subdivide if already at finest level
            if (leaf->level == 0) continue;

            for (int dir = 0; dir < 4; ++dir)
            {
                uint32_t minNeighbor = GetMinNeighborLevel(leaf, dir);
                // Neighbor is finer (lower level number) by more than 1 → subdivide
                if (minNeighbor + 1 < leaf->level)
                {
                    Subdivide(leaf);
                    changed = true;
                    break;  // leaf is no longer a leaf, move to next
                }
            }
        }
    }
}

//=============================================================================
// Update - rebuild quadtree and regenerate mesh
//=============================================================================

void QuadTreeTerrain::Update(const Vector3f& cameraPos,
                              float fovY,
                              float screenHeight)
{
    if (!mMesh) return;

    // Cache camera parameters for SSE
    if (fovY > 0.0f)
    {
        const float DEG2RAD = 0.0174532925199432958f;
        mTanHalfFovY = tanf(fovY * DEG2RAD * 0.5f);
    }
    if (screenHeight > 0.0f)
    {
        mScreenHeight = screenHeight;
    }

    // Update quadtree from root
    UpdateNode(&mRoot, cameraPos);

    // Enforce neighbor constraint: adjacent leaves must differ by at most 1 level
    EnforceNeighborConstraint();

    // Collect all leaf nodes
    mLeafNodes.clear();
    CollectLeaves(&mRoot);

    // Compute neighbor LOD info for each leaf (for crack-fixing fan)
    mLeafNeighborInfo.resize(mLeafNodes.size());
    for (size_t i = 0; i < mLeafNodes.size(); ++i)
    {
        Node* leaf = mLeafNodes[i];
        LeafNeighborInfo& info = mLeafNeighborInfo[i];

        // direction: 0=left(-X), 1=right(+X), 2=bottom(+Z), 3=top(-Z)
        uint32_t nbrLeft   = GetMinNeighborLevel(leaf, 0);
        uint32_t nbrRight  = GetMinNeighborLevel(leaf, 1);
        uint32_t nbrBottom = GetMinNeighborLevel(leaf, 2);
        uint32_t nbrTop    = GetMinNeighborLevel(leaf, 3);

        // Neighbor is coarser if its level is HIGHER than ours (larger number = coarser)
        info.leftCoarser   = (nbrLeft   > leaf->level) ? 1 : 0;
        info.rightCoarser  = (nbrRight  > leaf->level) ? 1 : 0;
        info.topCoarser    = (nbrTop    > leaf->level) ? 1 : 0;
        info.bottomCoarser = (nbrBottom > leaf->level) ? 1 : 0;
    }

    // Build mesh from leaves
    GenerateLeafMesh();
}

//=============================================================================
// GetHeight - bilinear interpolation of heightmap
//=============================================================================

float QuadTreeTerrain::GetHeight(float worldX, float worldZ) const
{
    if (mHeightMap.empty()) return 0.0f;

    float halfSize = mWorldSize * 0.5f;
    float step     = mWorldSize / (float)(mGridSize - 1);

    float fx = (worldX + halfSize) / step;
    float fz = (worldZ + halfSize) / step;

    int ix = (int)fx;
    int iz = (int)fz;

    ix = std::max(0, std::min(ix, (int)mGridSize - 2));
    iz = std::max(0, std::min(iz, (int)mGridSize - 2));

    float tx = fx - (float)ix;
    float tz = fz - (float)iz;

    float h00 = mHeightMap[iz * mGridSize + ix];
    float h10 = mHeightMap[iz * mGridSize + ix + 1];
    float h01 = mHeightMap[(iz + 1) * mGridSize + ix];
    float h11 = mHeightMap[(iz + 1) * mGridSize + ix + 1];

    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;

    return h0 * (1.0f - tz) + h1 * tz;
}

//=============================================================================
// Configuration setters
//=============================================================================

void QuadTreeTerrain::SetLODDistanceFactor(float factor)
{
    mLODDistanceFactor = factor;
}

void QuadTreeTerrain::SetSSEThreshold(float threshold)
{
    mSSEThreshold = threshold;
}

//=============================================================================
// BuildIndirectCommands - build indirect draw commands from visible leaves.
// Applies per-leaf frustum culling and fills the command buffer for a single
// DrawIndexedPrimitivesIndirect call.
//=============================================================================

void QuadTreeTerrain::BuildIndirectCommands(const mathutil::Frustumf* frustum)
{
    mIndirectCommands.clear();

    if (!mMesh) return;

    int subMeshCount = mMesh->GetSubMeshCount();
    const auto& leafBounds = GetLeafBounds();

    mIndirectCommands.reserve(subMeshCount);

    for (int n = 0; n < subMeshCount; n++)
    {
        // Per-leaf frustum culling
        if (frustum && n < (int)leafBounds.size())
        {
            if (!frustum->IsBoxInFrustum(leafBounds[n]))
                continue;
        }

        const SubMeshInfo& subInfo = mMesh->GetSubMeshInfo(n);
        RenderCore::DrawIndexedIndirectCommand cmd;
        cmd.indexCount    = subInfo.indexCount;
        cmd.instanceCount = 1;
        cmd.firstIndex    = subInfo.firstIndex;
        cmd.vertexOffset  = subInfo.baseVertex;
        cmd.firstInstance = 0;
        mIndirectCommands.push_back(cmd);
    }
}

NS_RENDERSYSTEM_END
