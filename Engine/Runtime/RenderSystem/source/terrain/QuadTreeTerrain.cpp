//
//  QuadTreeTerrain.cpp
//  GNXEngine
//
//  基于四叉树的自适应LOD地形系统。
//  功能：基于距离的LOD选择、三角形扇形裂缝修复、
//       静态索引池（零每帧GPU上传）、视锥体剔除、GPU驱动渲染路径。
//

#include "terrain/QuadTreeTerrain.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <unordered_map>

#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//=============================================================================
// 程序化高度函数（与 GeoMipTerrain 相同）
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
// 构造函数 / 析构函数
//=============================================================================

QuadTreeTerrain::QuadTreeTerrain() = default;
QuadTreeTerrain::~QuadTreeTerrain() = default;

//=============================================================================
// 工厂方法：从高度图创建地形
//=============================================================================

QuadTreeTerrainPtr QuadTreeTerrain::CreateFromHeightMap(
    const char* heightmapPath,
    float worldSizeXZ,
    float heightScale,
    uint32_t maxLevel)
{
    using namespace imagecodec;

    // 加载高度图
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

    // 将网格大小向上取整到 2^n + 1
    uint32_t n = 0;
    uint32_t cells = imageWidth - 1;
    while ((1u << n) < cells) ++n;
    uint32_t gridSize = (1u << n) + 1;

    auto terrain = QuadTreeTerrainPtr(new QuadTreeTerrain());
    terrain->mWorldSize     = worldSizeXZ;
    terrain->mHeightScale   = heightScale;
    terrain->mGridSize      = gridSize;
    terrain->mGridSizeCells = gridSize - 1;

    // 限制 maxLevel，确保最小节点尺寸 >= (kLeafVerticesPerSide - 1)
    uint32_t minNodeCells = kLeafVerticesPerSide - 1;  // 16
    uint32_t maxPossibleLevel = 0;
    uint32_t tmp = terrain->mGridSizeCells;
    while (tmp > minNodeCells) { tmp >>= 1; ++maxPossibleLevel; }
    terrain->mMaxLevel = std::min(maxLevel, maxPossibleLevel);

    LOG_INFO("QuadTreeTerrain: Heightmap %s (%ux%u), grid=%u, maxLevel=%u",
             heightmapPath, imageWidth, imageHeight, gridSize, terrain->mMaxLevel);

    // 采样高度图到 mHeightMap
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

    // 生成顶点数据
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

    // 通过中心差分计算法线（CreateFromHeightMap 路径）
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

    // 计算切线
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

    // 构建四叉树
    terrain->BuildNode(&terrain->mRoot);

    // 预计算几何误差（用于LOD选择）
    terrain->ComputeAllGeoErrors();

    // 在首次 Update() 之前构建静态索引池
    // （Update→GenerateLeafMesh 会读取 mIndexPool，必须先填充）
    terrain->BuildStaticIndexPool();

    // 将高度图上传为 GPU 纹理（阶段1A: 纯增量）
    terrain->CreateHeightMapTexture();

    // 创建用于 GPU 驱动渲染的模板网格
    terrain->CreateTemplateMesh();

    // 初始更新（此时安全 — 索引池已就绪）
    terrain->Update(Vector3f(0, 0, 0));

    LOG_INFO("QuadTreeTerrain: Created from heightmap, grid=%u, maxLevel=%u",
             gridSize, terrain->mMaxLevel);

    return terrain;
}

//=============================================================================
// 工厂方法：从程序化噪声创建地形
//=============================================================================

QuadTreeTerrainPtr QuadTreeTerrain::Create(
    uint32_t gridSize,
    float worldSizeXZ,
    float heightScale,
    uint32_t maxLevel)
{
    // 将 gridSize 向上取整到 2^n + 1
    uint32_t n = 0;
    uint32_t cells = gridSize - 1;
    while ((1u << n) < cells) ++n;
    uint32_t adjustedGridSize = (1u << n) + 1;

    auto terrain = QuadTreeTerrainPtr(new QuadTreeTerrain());
    terrain->mWorldSize     = worldSizeXZ;
    terrain->mHeightScale   = heightScale;
    terrain->mGridSize      = adjustedGridSize;
    terrain->mGridSizeCells = adjustedGridSize - 1;

    // 限制 maxLevel
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

    // 使用程序化高度生成顶点
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

    // 通过中心差分计算法线（Create 路径）
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

    // 计算切线
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

    // 构建四叉树
    terrain->BuildNode(&terrain->mRoot);

    // 预计算几何误差（用于 LOD 选择）
    terrain->ComputeAllGeoErrors();

    // 在首次 Update() 之前构建静态索引池
    terrain->BuildStaticIndexPool();

    // 初始更新（此时安全 — 索引池已就绪）
    terrain->Update(Vector3f(0, 0, 0));

    LOG_INFO("QuadTreeTerrain: Created procedural, grid=%u, maxLevel=%u",
             adjustedGridSize, terrain->mMaxLevel);

    return terrain;
}

//=============================================================================
    // 初始化顶点数据并创建 GPU 顶点缓冲
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
// CreateHeightMapTexture - 将 mHeightMap 上传为 R32Float Texture2D
// 仅在初始化时调用一次。该纹理对 Shader 只读。
//=============================================================================

void QuadTreeTerrain::CreateHeightMapTexture()
{
    if (mHeightMap.empty()) return;

    auto renderDevice = RenderCore::GetRenderDevice();

    // 创建与高度图尺寸匹配的 R32Float 纹理
    mHeightMapTexture = renderDevice->CreateTexture2D(
        RenderCore::kTexFormatR32Float,
        RenderCore::TextureUsage::TextureUsageShaderRead,
        mGridSize, mGridSize, 1);

    if (!mHeightMapTexture) return;

    // 上传高度数据：每像素 4 字节（R32Float）
    uint32_t bytesPerRow = mGridSize * sizeof(float);
    RenderCore::Rect2D region(0, 0, mGridSize, mGridSize);
    mHeightMapTexture->ReplaceRegion(region, 0,
        reinterpret_cast<const uint8_t*>(mHeightMap.data()), bytesPerRow);

    LOG_INFO("QuadTreeTerrain: Heightmap texture uploaded (%ux%u, R32Float)",
             mGridSize, mGridSize);
}

//=============================================================================
// BuildPatchMetaBuffer - 将叶节点元数据打包到 PatchMeta[] SSBO 中。
// 每帧在 GenerateLeafMesh 之后调用。该缓冲区随后可用于 GPU 驱动的渲染流程
//（compute 剔除、实例化绘制等）。
//=============================================================================

void QuadTreeTerrain::BuildPatchMetaBuffer()
{
    mPatchMetaData.clear();
    mPatchMetaData.reserve(mLeafNodes.size());

    float step = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    for (size_t i = 0; i < mLeafNodes.size(); ++i)
    {
        Node* leaf = mLeafNodes[i];

        PatchMeta meta;
        meta.worldX    = -halfSize + (float)leaf->x * step;
        meta.worldZ    = -halfSize + (float)leaf->z * step;
        meta.worldSize = (float)leaf->size * step;
        meta.minHeight = leaf->bounds.minimum.y;
        meta.gridX     = leaf->x;
        meta.gridZ     = leaf->z;
        meta.gridSize  = leaf->size;
        meta.level     = leaf->level;

        mPatchMetaData.push_back(meta);
    }

    // 上传到 GPU 作为 SSBO
    if (!mPatchMetaData.empty())
    {
        uint32_t dataSize = (uint32_t)(mPatchMetaData.size() * sizeof(PatchMeta));
        RenderCore::RCBufferDesc desc(dataSize,
            RenderCore::RCBufferUsage::StorageBuffer,
            RenderCore::StorageModeShared);  // CPU-accessible for creation

        auto renderDevice = RenderCore::GetRenderDevice();
        mPatchMetaBuffer = renderDevice->CreateBuffer(desc, mPatchMetaData.data());
    }
    else
    {
        mPatchMetaBuffer.reset();
    }
}

//=============================================================================
// CreateTemplateMesh - 创建 17x17 模板网格，用于 GPU 驱动渲染
//=============================================================================

void QuadTreeTerrain::CreateTemplateMesh()
{
    constexpr uint32_t kVertsPerSide = kLeafVerticesPerSide;  // 17
    constexpr uint32_t kCellCount   = kVertsPerSide - 1;      // 16
    constexpr uint32_t kVertexCount = kVertsPerSide * kVertsPerSide;  // 289
    constexpr uint32_t kIndexCount  = kCellCount * kCellCount * 6;     // 1536

    // 构建 SoA 布局的顶点数据：先位置，后纹理坐标
    std::vector<float> positions(kVertexCount * 3);
    std::vector<float> texCoords(kVertexCount * 2);

    for (uint32_t row = 0; row < kVertsPerSide; ++row)
    {
        for (uint32_t col = 0; col < kVertsPerSide; ++col)
        {
            uint32_t idx = row * kVertsPerSide + col;
            float u = (float)col / (float)kCellCount;  // [0, 1]
            float v = (float)row / (float)kCellCount;  // [0, 1]

            // 位置：局部 XZ 坐标，Y=0（在顶点着色器中由高度图替换）
            positions[idx * 3 + 0] = u;  // 局部 X → 通过 PatchMeta 映射到世界 X
            positions[idx * 3 + 1] = 0.0f;
            positions[idx * 3 + 2] = v;  // 局部 Z → 通过 PatchMeta 映射到世界 Z

            // 纹理坐标：与局部 UV 相同
            texCoords[idx * 2 + 0] = u;
            texCoords[idx * 2 + 1] = v;
        }
    }

    // 构建索引数据
    std::vector<uint32_t> indices(kIndexCount);
    uint32_t idxOffset = 0;
    for (uint32_t row = 0; row < kCellCount; ++row)
    {
        for (uint32_t col = 0; col < kCellCount; ++col)
        {
            uint32_t v00 = row * kVertsPerSide + col;
            uint32_t v10 = v00 + 1;
            uint32_t v01 = v00 + kVertsPerSide;
            uint32_t v11 = v01 + 1;

            indices[idxOffset++] = v00;
            indices[idxOffset++] = v01;
            indices[idxOffset++] = v11;

            indices[idxOffset++] = v00;
            indices[idxOffset++] = v11;
            indices[idxOffset++] = v10;
        }
    }

    // 创建 SoA 顶点缓冲区：[位置 | 纹理坐标]
    uint32_t posDataSize  = kVertexCount * 3 * sizeof(float);  // 3468 bytes
    uint32_t uvDataSize   = kVertexCount * 2 * sizeof(float);  // 2312 bytes
    uint32_t totalVBSize  = posDataSize + uvDataSize;
    mTemplatePositionSize = posDataSize;

    std::vector<uint8_t> vbData(totalVBSize);
    memcpy(vbData.data(), positions.data(), posDataSize);
    memcpy(vbData.data() + posDataSize, texCoords.data(), uvDataSize);

    auto renderDevice = RenderCore::GetRenderDevice();

    // 创建顶点缓冲区（RCBuffer，VertexBuffer 用途）
    RenderCore::RCBufferDesc vbDesc(totalVBSize,
        RenderCore::RCBufferUsage::VertexBuffer,
        RenderCore::StorageModeShared);
    mTemplateVB = renderDevice->CreateBuffer(vbDesc, vbData.data());
    if (mTemplateVB)
        mTemplateVB->SetName("TerrainTemplateVB");

    // 创建索引缓冲区
    mTemplateIB = renderDevice->CreateIndexBufferWithBytes(indices.data(), kIndexCount * sizeof(uint32_t), RenderCore::IndexType_UInt);
}

//=============================================================================
// BuildGPUPathData - 构建可见 PatchMeta SSBO + 间接绘制命令
//=============================================================================

void QuadTreeTerrain::BuildGPUPathData(const mathutil::Frustumf* frustum)
{
    mVisiblePatchMeta.clear();

    const auto& leafBounds = GetLeafBounds();
    float step = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    for (size_t i = 0; i < mLeafNodes.size(); ++i)
    {
        // 视锥体剔除
        if (frustum && i < leafBounds.size())
        {
            if (!frustum->IsBoxInFrustum(leafBounds[i]))
                continue;
        }

        Node* leaf = mLeafNodes[i];

        // 构建可见 PatchMeta
        PatchMeta meta;
        meta.worldX    = -halfSize + (float)leaf->x * step;
        meta.worldZ    = -halfSize + (float)leaf->z * step;
        meta.worldSize = (float)leaf->size * step;
        meta.minHeight = leaf->bounds.minimum.y;
        meta.gridX     = leaf->x;
        meta.gridZ     = leaf->z;
        meta.gridSize  = leaf->size;
        meta.level     = leaf->level;
        mVisiblePatchMeta.push_back(meta);
    }

    // 上传可见 PatchMeta 到 GPU 作为 SSBO — 复用缓冲区以避免 GPU 同步问题
    if (!mVisiblePatchMeta.empty())
    {
        uint32_t dataSize = (uint32_t)(mVisiblePatchMeta.size() * sizeof(PatchMeta));
        auto renderDevice = RenderCore::GetRenderDevice();
        
        // 尽可能复用已有缓冲区，避免重新分配
        if (mVisiblePatchMetaBuffer && mVisiblePatchMetaBuffer->GetSize() >= dataSize)
        {
            // 更新已有缓冲区内容 — 避免 GPU 同步问题
            void* mappedData = mVisiblePatchMetaBuffer->Map();
            if (mappedData)
            {
                memcpy(mappedData, mVisiblePatchMeta.data(), dataSize);
                mVisiblePatchMetaBuffer->Unmap();
            }
        }
        else
        {
            // 仅在必要时重新创建缓冲区
            RenderCore::RCBufferDesc desc(dataSize,
                RenderCore::RCBufferUsage::StorageBuffer,
                RenderCore::StorageModeShared);
            mVisiblePatchMetaBuffer = renderDevice->CreateBuffer(desc, mVisiblePatchMeta.data());
        }
    }
    else
    {
        // 即使没有可见 patch 也保留缓冲区，以维持同步一致性
        if (mVisiblePatchMetaBuffer)
        {
            // 清空缓冲区内容但保留分配
            static PatchMeta emptyMeta = {};
            void* mappedData = mVisiblePatchMetaBuffer->Map();
            if (mappedData)
            {
                memcpy(mappedData, &emptyMeta, sizeof(PatchMeta));
                mVisiblePatchMetaBuffer->Unmap();
            }
        }
    }
}

//=============================================================================
// BuildNode - 构建四叉树根节点
//=============================================================================

void QuadTreeTerrain::BuildNode(Node* node)
{
    node->x     = 0;
    node->z     = 0;
    node->size  = mGridSizeCells;
    node->level = mMaxLevel;  // 根节点从最粗 LOD 开始（mMaxLevel 为最粗，0 为最细）
    ComputeNodeBounds(node);
}

//=============================================================================
// ComputeNodeBounds - 计算节点的世界空间 AABB
//=============================================================================

void QuadTreeTerrain::ComputeNodeBounds(Node* node)
{
    float step     = mWorldSize / (float)(mGridSize - 1);
    float halfSize = mWorldSize * 0.5f;

    // 网格范围 [x, x+size] 个单元 → 顶点范围 [x, x+size]
    float worldMinX = -halfSize + (float)node->x * step;
    float worldMinZ = -halfSize + (float)node->z * step;
    float worldMaxX = -halfSize + (float)(node->x + node->size) * step;
    float worldMaxZ = -halfSize + (float)(node->z + node->size) * step;

    // 在该节点区域内查找最小/最大高度
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
// ShouldSubdivide - 判断节点是否应该分裂为 4 个子节点
//=============================================================================

bool QuadTreeTerrain::ShouldSubdivide(const Node& node, const Vector3f& cameraPos) const
{
    // 不能超过最细层级继续细分（0=最细，不能再往下）
    if (node.level == 0) return false;

    // 基于距离的 LOD：相机足够近时细分。
    // 较大的节点在更远的距离就细分，较小的节点只在很近时才细分。
    float nodeWorldSize = (float)node.size * (mWorldSize / (float)(mGridSize - 1));
    float threshold = nodeWorldSize * mLODDistanceFactor;

    float dx = cameraPos.x - node.bounds.center.x;
    float dz = cameraPos.z - node.bounds.center.z;
    float dy = cameraPos.y;
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);

    return distance < threshold;
}

//=============================================================================
// Subdivide - 为节点创建 4 个子节点
//=============================================================================

void QuadTreeTerrain::Subdivide(Node* node)
{
    uint32_t halfSize = node->size / 2;
    uint32_t childLevel = node->level - 1;  // 变细：5→4→3→2→1→0（0=最细，mMaxLevel=最粗）

    // 西北（左上）
    node->children[0] = std::make_unique<Node>();
    node->children[0]->x     = node->x;
    node->children[0]->z     = node->z;
    node->children[0]->size  = halfSize;
    node->children[0]->level = childLevel;
    node->children[0]->parent = node;
    ComputeNodeBounds(node->children[0].get());
    node->children[0]->maxGeoError = GetCachedGeoError(childLevel, node->x, node->z);

    // 东北（右上）
    node->children[1] = std::make_unique<Node>();
    node->children[1]->x     = node->x + halfSize;
    node->children[1]->z     = node->z;
    node->children[1]->size  = halfSize;
    node->children[1]->level = childLevel;
    node->children[1]->parent = node;
    ComputeNodeBounds(node->children[1].get());
    node->children[1]->maxGeoError = GetCachedGeoError(childLevel, node->x + halfSize, node->z);

    // 西南（左下）
    node->children[2] = std::make_unique<Node>();
    node->children[2]->x     = node->x;
    node->children[2]->z     = node->z + halfSize;
    node->children[2]->size  = halfSize;
    node->children[2]->level = childLevel;
    node->children[2]->parent = node;
    ComputeNodeBounds(node->children[2].get());
    node->children[2]->maxGeoError = GetCachedGeoError(childLevel, node->x, node->z + halfSize);

    // 东南（右下）
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
// UpdateNode - 根据相机位置递归更新四叉树
//=============================================================================

void QuadTreeTerrain::UpdateNode(Node* node, const Vector3f& cameraPos)
{
    if (ShouldSubdivide(*node, cameraPos))
    {
        // 需要细分
        if (node->IsLeaf())
        {
            Subdivide(node);
        }

        // 递归进入子节点
        for (int i = 0; i < 4; ++i)
        {
            UpdateNode(node->children[i].get(), cameraPos);
        }
    }
    else
    {
        // 不需要细分 — 如果有子节点则合并
        for (int i = 0; i < 4; ++i)
        {
            node->children[i].reset();
        }
    }
}

//=============================================================================
// CollectLeaves - 收集所有叶节点到 mLeafNodes 中
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
// GenerateLeafMesh - 从当前叶节点构建 SubMeshInfo 列表。
//
// 从静态索引池（由 BuildStaticIndexPool 构建）中查找预计算的索引缓冲区。
// 无需索引生成，无需 GPU 上传 — 每个叶节点 O(1)。
//
// 静态池中的所有索引都是叶节点局部的。我们为每个叶节点设置 baseVertex
// 来将它们偏移到全局地形顶点缓冲区的正确位置。
//=============================================================================

void QuadTreeTerrain::GenerateLeafMesh()
{
    // 清除上一帧的 SubMeshInfos
    mMesh->ClearSubMeshInfos();
    mLeafBounds.clear();
    mLeafBounds.reserve(mLeafNodes.size());

    for (size_t idx = 0; idx < mLeafNodes.size(); ++idx)
    {
        Node* leaf = mLeafNodes[idx];
        const LeafNeighborInfo& nbrInfo = mLeafNeighborInfo[idx];

        // 计算该叶节点 LOD 层级的步长
        uint32_t cellsPerSide = kLeafVerticesPerSide - 1;  // 16
        uint32_t stride = leaf->size / cellsPerSide;

        // 将步长映射到静态池中的层级索引
        uint32_t strideLevel = GetStrideLevel(stride);

        // 将 4 个邻居更粗标志编码为排列索引 [0..15]
        uint32_t perm = (nbrInfo.leftCoarser   ? 1u : 0u)
                      | (nbrInfo.rightCoarser  ? 2u : 0u)
                      | (nbrInfo.topCoarser    ? 4u : 0u)
                      | (nbrInfo.bottomCoarser ? 8u : 0u);

        // 从静态池中查找预计算的索引
        const IndexPoolEntry& entry = mIndexPool[strideLevel][perm];

        SubMeshInfo subMeshInfo;
        subMeshInfo.firstIndex = entry.start;
        subMeshInfo.indexCount = entry.count;
        subMeshInfo.topology   = PrimitiveMode_TRIANGLES;
        subMeshInfo.vertexCount = mGridSize * mGridSize;
        // baseVertex 将叶节点局部索引偏移到全局 VB 中的绝对位置
        subMeshInfo.baseVertex = leaf->z * mGridSize + leaf->x;
        mMesh->AddSubMeshInfo(subMeshInfo);
        mLeafBounds.push_back(leaf->bounds);
    }

    // 不调用 UpdateIndices — 缓冲区是静态的，在初始化时构建一次
}

//=============================================================================
// CreateTriangleFanLocal - 为单个扇形单元生成带裂缝修复的三角形。
//
// 输出半局部索引：使用全局行跨度（mGridSize）但叶节点局部坐标。
// 这使得 baseVertex 能正确偏移到全局 VB 中：
//
//   finalIndex = poolIndex + baseVertex
//             = (local_z * mGridSize + local_x) + (leaf->z * mGridSize + leaf->x)
//             = (leaf->z + local_z) * mGridSize + (leaf->x + local_x)  ✓
//
// (fcx, fcz): 扇形单元原点（叶节点局部网格坐标）
// globalGridSize: mGridSize（全局地形每边顶点数）
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
    // 各方向有效步长：邻居更粗时步长翻倍
    uint32_t sLeft   = leftCoarser   ? 2 * stride : stride;
    uint32_t sRight  = rightCoarser  ? 2 * stride : stride;
    uint32_t sTop    = topCoarser    ? 2 * stride : stride;
    uint32_t sBottom = bottomCoarser ? 2 * stride : stride;

    // 扇形中心（半局部坐标：全局行跨度，叶节点局部偏移）
    uint32_t idxCenter = (fcz + stride) * globalGridSize + (fcx + stride);

    // ---- 扇区 1：左边缘（从 TL 到 BL），向下遍历 ----
    uint32_t t1 = fcz * globalGridSize + fcx;
    uint32_t t2 = (fcz + sLeft) * globalGridSize + fcx;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!leftCoarser)
    {
        t1 = t2; t2 += sLeft * globalGridSize;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- 扇区 2：下边缘（从 BL 到 BR），向右遍历 ----
    // 接续扇区 1 的终点
    t1 = t2; t2 = t1 + sBottom;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!bottomCoarser)
    {
        t1 = t2; t2 += sBottom;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- 扇区 3：右边缘（从 BR 到 TR），向上遍历 ----
    // 接续扇区 2 的终点
    t1 = t2; t2 = t1 - sRight * globalGridSize;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!rightCoarser)
    {
        t1 = t2; t2 -= sRight * globalGridSize;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }

    // ---- 扇区 4：上边缘（从 TR 到 TL），向左遍历 ----
    // 接续扇区 3 的终点
    t1 = t2; t2 = t1 - sTop;
    indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);

    if (!topCoarser)
    {
        t1 = t2; t2 -= sTop;
        indices.push_back(idxCenter); indices.push_back(t1); indices.push_back(t2);
    }
}

//=============================================================================
// GetStrideLevel - 将步长值映射为层级索引（log2）。
// 步长必须是 2 的幂：stride=1 返回 0，stride=2 返回 1，以此类推。
//=============================================================================

uint32_t QuadTreeTerrain::GetStrideLevel(uint32_t stride) const
{
    uint32_t level = 0;
    while (stride > 1) { stride >>= 1; ++level; }
    return level;
}

//=============================================================================
// ComputeNodeGeoError - 计算节点以其 LOD 层级渲染时的最大几何误差
//（即每边使用 kLeafVerticesPerSide 个顶点）。
//
// 几何误差是该节点内所有不在渲染顶点网格上的网格顶点的
// |实际高度 - 双线性插值高度| 的最大值。
// 当节点大小等于 kLeafVerticesPerSide-1 时，每个顶点都被渲染，误差为零。
//=============================================================================

float QuadTreeTerrain::ComputeNodeGeoError(uint32_t x, uint32_t z, uint32_t size) const
{
    uint32_t cellsPerSide = kLeafVerticesPerSide - 1;  // 16
    if (size <= cellsPerSide) return 0.0f;  // 最细细节，无误差

    uint32_t stride = size / cellsPerSide;
    float maxError = 0.0f;

    for (uint32_t gz = z; gz <= z + size && gz < mGridSize; ++gz)
    {
        for (uint32_t gx = x; gx <= x + size && gx < mGridSize; ++gx)
        {
            // 跳过已渲染的顶点（它们恰好位于采样网格上）
            uint32_t localX = gx - x;
            uint32_t localZ = gz - z;
            if (localX % stride == 0 && localZ % stride == 0) continue;

            // 确定该顶点落在哪个渲染网格单元中
            uint32_t cellX = localX / stride;
            uint32_t cellZ = localZ / stride;

            // 钳制到有效单元范围
            cellX = std::min(cellX, cellsPerSide - 1);
            cellZ = std::min(cellZ, cellsPerSide - 1);

            // 渲染网格单元的 4 个角（全局坐标）
            uint32_t x0 = x + cellX * stride;
            uint32_t z0 = z + cellZ * stride;
            uint32_t x1 = std::min(x0 + stride, mGridSize - 1);
            uint32_t z1 = std::min(z0 + stride, mGridSize - 1);

            float h00 = mHeightMap[z0 * mGridSize + x0];
            float h10 = mHeightMap[z0 * mGridSize + x1];
            float h01 = mHeightMap[z1 * mGridSize + x0];
            float h11 = mHeightMap[z1 * mGridSize + x1];

            // 双线性插值权重
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
// ComputeAllGeoErrors - 预计算每个层级所有可能节点的几何误差。
// 在初始化时调用一次。
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

    // 设置根节点的 geoError（根层级 = 0，最粗）
    mRoot.maxGeoError = GetCachedGeoError(mRoot.level, mRoot.x, mRoot.z);

    LOG_INFO("QuadTreeTerrain: GeoError cache computed for %u levels", mMaxLevel + 1);
}

//=============================================================================
// GetCachedGeoError - 从缓存中查找预计算的几何误差
//=============================================================================

float QuadTreeTerrain::GetCachedGeoError(uint32_t level, uint32_t x, uint32_t z) const
{
    // 将 LOD 层级转换为深度索引：depth=mMaxLevel 为最粗（根节点），depth=0 为最细
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
// BuildStaticIndexPool - 为静态 IB 预计算所有索引排列。
//
// 对每个可能的步长层级（1, 2, 4, ..., maxStride），生成全部 16 种
// 邻居更粗排列。每个条目包含一个叶节点配置的完整索引缓冲区
//（64 个扇形单元 × 每个 4~8 个三角形）。
//
// 所有索引都是叶节点局部的。运行时，GenerateLeafMesh() 查找正确的条目
// 并设置 baseVertex 来偏移到全局 VB 中。
//
// 在地形创建时调用一次。此后不再调用 UpdateIndices() — 零每帧 GPU 上传。
//=============================================================================

void QuadTreeTerrain::BuildStaticIndexPool()
{
    uint32_t minNodeCells = kLeafVerticesPerSide - 1;  // 16
    uint32_t maxStride = mGridSizeCells / minNodeCells;

    // 计算最大步长层级（maxStride 的 log2）
    uint32_t maxStrideLevel = 0;
    { uint32_t tmp = maxStride; while (tmp > 1) { tmp >>= 1; ++maxStrideLevel; } }

    uint32_t numLevels = maxStrideLevel + 1;
    mIndexPool.resize(numLevels);
    for (auto& v : mIndexPool) v.resize(16);

    mMasterIndices.clear();
    mMasterIndices.reserve(numLevels * 16 * 1200);  // ~1200 索引/条目（估算）

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

            // 为该叶节点配置生成全部 64 个扇形单元
            for (uint32_t fj = 0; fj < fanCellsPerSide; ++fj)
            {
                for (uint32_t fi = 0; fi < fanCellsPerSide; ++fi)
                {
                    // 扇形单元原点（叶节点局部坐标）
                    uint32_t fcx = fi * 2 * stride;
                    uint32_t fcz = fj * 2 * stride;

                    // 叶节点扇形单元网格内的边缘检测
                    // fj=0 → fcz=0 → 最小 Z → 上边缘（-Z）
                    // fj=max → 最大 Z → 下边缘（+Z）
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

    // 上传主索引缓冲区到 GPU — 仅一次
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
// GetChildIndex - 查找该节点在其父节点中的子节点索引
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
// GetMinNeighborLevel - 查找 `node` 在指定边上所有相邻叶节点中的
// 最小（最细）层级。
//
// direction: 0=左(-X), 1=右(+X), 2=下(+Z), 3=上(-Z)
//
// 使用标准四叉树邻居查找算法：
// 1. 向上遍历树，找到其兄弟节点在目标方向的祖先。
// 2. 下降进入该兄弟节点，沿着共享边的子节点，找到最细的叶节点。
//=============================================================================

uint32_t QuadTreeTerrain::GetMinNeighborLevel(const Node* node, int direction) const
{
    // 子节点布局：
    //   0=NW(x,    z,    h)  1=NE(x+h, z,    h)
    //   2=SW(x,    z+h,  h)  3=SE(x+h, z+h,  h)
    //
    // 对每个方向，某些子节点的邻居是兄弟节点，
    // 其他子节点需要先向上到父节点。
    //
    // 左(-X):   子节点1→兄弟0, 子节点3→兄弟2;  子节点0,2→上溯
    // 右(+X):   子节点0→兄弟1, 子节点2→兄弟3;  子节点1,3→上溯
    // 下(+Z):   子节点0→兄弟2, 子节点1→兄弟3;  子节点2,3→上溯
    // 上(-Z):   子节点2→兄弟0, 子节点3→兄弟1;  子节点0,1→上溯

    static const int kSibling[4][4] = {
        { -1,  0, -1,  2 },   // LEFT:  child 1→0, child 3→2
        {  1, -1,  3, -1 },   // RIGHT: child 0→1, child 2→3
        {  2,  3, -1, -1 },   // BOTTOM:child 0→2, child 1→3
        { -1, -1,  0,  1 },   // TOP:   child 2→0, child 3→1
    };

    // 下降到邻居时要探索的子节点。
    // 左:   右侧子节点 [1,3]（它们共享邻居的左边缘）
    // 右:   左侧子节点 [0,2]
    // 下:   上侧子节点 [0,1]
    // 上:   下侧子节点 [2,3]
    static const int kDescend[4][2] = {
        { 1, 3 },   // LEFT
        { 0, 2 },   // RIGHT
        { 0, 1 },   // BOTTOM
        { 2, 3 },   // TOP
    };

    // 向上遍历以找到相同或更粗层级的邻居
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
        // 地形边界 — 该方向无邻居
        return node->level;
    }

    // 下降到邻居树中，找到与原节点共享边的所有叶节点中的最小层级（最细细节）。
    //
    // 我们必须只沿着垂直轴范围与原节点范围重叠的子节点前进。
    // 否则可能会统计到实际上不共享边的叶节点。

    uint32_t origMin, origMax;  // 原节点在垂直轴上的范围
    if (direction <= 1)  // 左/右 → 垂直轴为 Z
    {
        origMin = node->z;
        origMax = node->z + node->size;
    }
    else  // 下/上 → 垂直轴为 X
    {
        origMin = node->x;
        origMax = node->x + node->size;
    }

    uint32_t minLevel = UINT32_MAX;  // 从最细可能值开始，但我们会找到最细层级（0=最细，mMaxLevel=最粗）

    // DFS 深度优先搜索进入邻居树
    std::vector<const Node*> stack;
    stack.push_back(neighbor);

    while (!stack.empty())
    {
        const Node* n = stack.back();
        stack.pop_back();

        // 检查该节点是否与原节点的垂直范围重叠
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
            continue;  // 无重叠，跳过
        }

        if (n->IsLeaf())
        {
            minLevel = std::min(minLevel, n->level);  // 找到最细层级（0=最细，mMaxLevel=最粗）
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
// EnforceNeighborConstraint - 确保相邻叶节点层级差不超过 1。
//
// 在基于相机的细分后，相邻叶节点的层级差可能达到 2+（例如
// level-1 叶节点旁边是 level-3 叶节点）。此过程强制将邻居比自身
// 细 1 级以上的叶节点细分，直到约束满足为止。
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
            // 已在最细层级，无法继续细分
            if (leaf->level == 0) continue;

            for (int dir = 0; dir < 4; ++dir)
            {
                uint32_t minNeighbor = GetMinNeighborLevel(leaf, dir);
                // 邻居比自身细（层级号更小）超过 1 级 → 细分
                if (minNeighbor + 1 < leaf->level)
                {
                    Subdivide(leaf);
                    changed = true;
                    break;  // 叶节点不再是叶节点，处理下一个
                }
            }
        }
    }
}

//=============================================================================
// Update - 重建四叉树并重新生成网格
//=============================================================================

void QuadTreeTerrain::Update(const Vector3f& cameraPos,
                              float fovY,
                              float screenHeight)
{
    if (!mMesh) return;

    // 缓存相机参数用于 SSE
    if (fovY > 0.0f)
    {
        const float DEG2RAD = 0.0174532925199432958f;
        mTanHalfFovY = tanf(fovY * DEG2RAD * 0.5f);
    }
    if (screenHeight > 0.0f)
    {
        mScreenHeight = screenHeight;
    }

    // 从根节点更新四叉树
    UpdateNode(&mRoot, cameraPos);

    // 强制邻居约束：相邻叶节点层级差最多为 1
    EnforceNeighborConstraint();

    // 收集所有叶节点
    mLeafNodes.clear();
    CollectLeaves(&mRoot);

    // 为每个叶节点计算邻居 LOD 信息（用于裂缝修复扇形）
    mLeafNeighborInfo.resize(mLeafNodes.size());
    for (size_t i = 0; i < mLeafNodes.size(); ++i)
    {
        Node* leaf = mLeafNodes[i];
        LeafNeighborInfo& info = mLeafNeighborInfo[i];

        // direction: 0=左(-X), 1=右(+X), 2=下(+Z), 3=上(-Z)
        uint32_t nbrLeft   = GetMinNeighborLevel(leaf, 0);
        uint32_t nbrRight  = GetMinNeighborLevel(leaf, 1);
        uint32_t nbrBottom = GetMinNeighborLevel(leaf, 2);
        uint32_t nbrTop    = GetMinNeighborLevel(leaf, 3);

        // 邻居更粗的条件：其层级号比我们大（数字越大 = 越粗）
        info.leftCoarser   = (nbrLeft   > leaf->level) ? 1 : 0;
        info.rightCoarser  = (nbrRight  > leaf->level) ? 1 : 0;
        info.topCoarser    = (nbrTop    > leaf->level) ? 1 : 0;
        info.bottomCoarser = (nbrBottom > leaf->level) ? 1 : 0;
    }

    // 从叶节点构建网格
    GenerateLeafMesh();

    // Build PatchMeta SSBO for GPU-driven rendering (阶段1A: 纯增量)
    BuildPatchMetaBuffer();
}

//=============================================================================
// GetHeight - 高度图的双线性插值查询
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
// 配置参数设置器
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
// BuildIndirectCommands - 从可见叶节点构建间接绘制命令。
// 对每个叶节点执行视锥体剔除，填充命令缓冲区用于单次
// DrawIndexedPrimitivesIndirect 调用。
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
        // 逐叶节点视锥体剔除
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
