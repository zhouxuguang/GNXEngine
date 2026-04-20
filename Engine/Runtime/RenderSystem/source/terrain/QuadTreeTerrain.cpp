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

    // Initial update
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

    // Initial update
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
    node->level = 0;
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
    // Cannot subdivide beyond maxLevel
    if (node.level >= mMaxLevel) return false;

    // Distance-based test: subdivide if camera is close enough
    float nodeWorldSize = (float)node.size * (mWorldSize / (float)(mGridSize - 1));
    float threshold = nodeWorldSize * mLODDistanceFactor;

    Vector3f nodeCenter = node.bounds.center;
    float dx = cameraPos.x - nodeCenter.x;
    float dy = cameraPos.y - nodeCenter.y;
    float dz = cameraPos.z - nodeCenter.z;
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);

    return distance < threshold;
}

//=============================================================================
// Subdivide - create 4 children for a node
//=============================================================================

void QuadTreeTerrain::Subdivide(Node* node)
{
    uint32_t halfSize = node->size / 2;
    uint32_t childLevel = node->level + 1;

    // NW (top-left)
    node->children[0] = std::make_unique<Node>();
    node->children[0]->x     = node->x;
    node->children[0]->z     = node->z;
    node->children[0]->size  = halfSize;
    node->children[0]->level = childLevel;
    node->children[0]->parent = node;
    ComputeNodeBounds(node->children[0].get());

    // NE (top-right)
    node->children[1] = std::make_unique<Node>();
    node->children[1]->x     = node->x + halfSize;
    node->children[1]->z     = node->z;
    node->children[1]->size  = halfSize;
    node->children[1]->level = childLevel;
    node->children[1]->parent = node;
    ComputeNodeBounds(node->children[1].get());

    // SW (bottom-left)
    node->children[2] = std::make_unique<Node>();
    node->children[2]->x     = node->x;
    node->children[2]->z     = node->z + halfSize;
    node->children[2]->size  = halfSize;
    node->children[2]->level = childLevel;
    node->children[2]->parent = node;
    ComputeNodeBounds(node->children[2].get());

    // SE (bottom-right)
    node->children[3] = std::make_unique<Node>();
    node->children[3]->x     = node->x + halfSize;
    node->children[3]->z     = node->z + halfSize;
    node->children[3]->size  = halfSize;
    node->children[3]->level = childLevel;
    node->children[3]->parent = node;
    ComputeNodeBounds(node->children[3].get());
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
// GenerateLeafMesh - build index buffer and SubMeshInfo from leaf nodes
//=============================================================================

void QuadTreeTerrain::GenerateLeafMesh()
{
    std::vector<uint32_t> indices;

    // Clear SubMeshInfos BEFORE UpdateIndices (so UpdateIndices doesn't
    // overwrite our SubMeshInfo entries)
    mMesh->ClearSubMeshInfos();
    mLeafBounds.clear();
    mLeafBounds.reserve(mLeafNodes.size());

    for (Node* leaf : mLeafNodes)
    {
        // Compute stride for this leaf's LOD level.
        // Each leaf renders kLeafVerticesPerSide vertices per side.
        // stride = leaf->size / (kLeafVerticesPerSide - 1)
        uint32_t cellsPerSide = kLeafVerticesPerSide - 1;
        uint32_t stride = leaf->size / cellsPerSide;
        uint32_t vps = leaf->size / stride + 1;  // should equal kLeafVerticesPerSide

        uint32_t indexStart = (uint32_t)indices.size();

        // Generate triangle indices for a regular grid
        for (uint32_t j = 0; j < vps - 1; ++j)
        {
            for (uint32_t i = 0; i < vps - 1; ++i)
            {
                // Absolute vertex indices in the shared vertex buffer
                uint32_t v00 = (leaf->z + j * stride) * mGridSize + (leaf->x + i * stride);
                uint32_t v10 = (leaf->z + j * stride) * mGridSize + (leaf->x + (i + 1) * stride);
                uint32_t v01 = (leaf->z + (j + 1) * stride) * mGridSize + (leaf->x + i * stride);
                uint32_t v11 = (leaf->z + (j + 1) * stride) * mGridSize + (leaf->x + (i + 1) * stride);

                // Two triangles per quad
                indices.push_back(v00);
                indices.push_back(v01);
                indices.push_back(v10);

                indices.push_back(v10);
                indices.push_back(v01);
                indices.push_back(v11);
            }
        }

        SubMeshInfo subMeshInfo;
        subMeshInfo.firstIndex = indexStart;
        subMeshInfo.indexCount = (uint32_t)indices.size() - indexStart;
        subMeshInfo.topology   = PrimitiveMode_TRIANGLES;
        subMeshInfo.vertexCount = mGridSize * mGridSize;
        subMeshInfo.baseVertex = 0;  // indices are already absolute
        mMesh->AddSubMeshInfo(subMeshInfo);
        mLeafBounds.push_back(leaf->bounds);
    }

    // Upload new index buffer to GPU
    if (!indices.empty())
    {
        mMesh->UpdateIndices(indices.data(), indices.size());
    }
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
// GetMaxNeighborLevel - find the maximum leaf level among all leaves
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

uint32_t QuadTreeTerrain::GetMaxNeighborLevel(const Node* node, int direction) const
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

    // Descend into the neighbor to find the maximum leaf level
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

    uint32_t maxLevel = 0;

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
            maxLevel = std::max(maxLevel, n->level);
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

    return maxLevel;
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
            for (int dir = 0; dir < 4; ++dir)
            {
                uint32_t maxNeighbor = GetMaxNeighborLevel(leaf, dir);
                if (maxNeighbor > leaf->level + 1)
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

NS_RENDERSYSTEM_END
