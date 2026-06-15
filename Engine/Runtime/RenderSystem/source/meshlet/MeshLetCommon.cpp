//
//  MeshLetCommon.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2024/6/14.
//

#include "meshlet/MeshLetCommon.h"

// meshopt 头文件
#include "meshoptimizer.h"

NS_RENDERSYSTEM_BEGIN

USING_NS_MATHUTIL

// =======================================================================
// BuildMeshlets
// =======================================================================

void BuildMeshlets(
    const uint32_t* indices,
    size_t          indexCount,
    const float*    positions,
    size_t          vertexCount,
    size_t          positionStride,
    std::vector<Meshlet>&           outMeshlets,
    std::vector<uint32_t>&          outVertices,
    std::vector<uint32_t>&          outTriangles)
{
    // 1. 计算最大可能的 meshlet 数量
    const size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, kMeshletMaxVertices, kMeshletMaxTriangles);

    // 2. 分配临时存储（meshopt 输出格式）
    std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
    std::vector<uint32_t>        meshletVertices(maxMeshlets * kMeshletMaxVertices);
    std::vector<uint8_t>         meshletTriangles(maxMeshlets * kMeshletMaxTriangles * 3);

    // 3. 构建 meshlet
    const size_t meshletCount = meshopt_buildMeshlets(
        meshlets.data(),
        meshletVertices.data(),
        meshletTriangles.data(),
        indices,
        indexCount,
        positions,
        vertexCount,
        positionStride,
        kMeshletMaxVertices,
        kMeshletMaxTriangles,
        0.0f);  // coneWeight = 0，暂不启用锥体剔除

    // 4. 修剪到实际使用的大小
    const auto& last = meshlets[meshletCount - 1];
    meshletVertices.resize(last.vertex_offset + last.vertex_count);
    meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    meshlets.resize(meshletCount);

    // 5. 将 meshopt 输出转换并重新打包到我们的 Meshlet 结构中
    outMeshlets.clear();
    outMeshlets.reserve(meshletCount);
    
    // Repack triangles from 3 consecutive byes to 4-byte uint32_t to
    // make it easier to unpack on the GPU.
    //
    std::vector<uint32_t> meshletTrianglesU32;
    for (auto& m : meshlets)
    {
        // Save triangle offset for current meshlet
        uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());

        // Repack to uint32_t
        for (uint32_t i = 0; i < m.triangle_count; ++i)
        {
            uint32_t i0 = 3 * i + 0 + m.triangle_offset;
            uint32_t i1 = 3 * i + 1 + m.triangle_offset;
            uint32_t i2 = 3 * i + 2 + m.triangle_offset;

            uint8_t  vIdx0  = meshletTriangles[i0];
            uint8_t  vIdx1  = meshletTriangles[i1];
            uint8_t  vIdx2  = meshletTriangles[i2];
            uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
                              ((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
                              ((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
            meshletTrianglesU32.push_back(packed);
        }

        // 添加 meshlet
        Meshlet meshlet = {};
        meshlet.vertexOffset   = m.vertex_offset;
        meshlet.triangleOffset = triangleOffset;
        meshlet.vertexCount    = m.vertex_count;
        meshlet.triangleCount  = m.triangle_count;
        outMeshlets.push_back(meshlet);
    }

    outVertices.swap(meshletVertices);
    outTriangles.swap(meshletTrianglesU32);
}

// =======================================================================
// ComputeMeshletBounds
// =======================================================================

void ComputeMeshletBounds(
    const float*    meshVertices,
    const uint32_t* meshletVerts,
    uint32_t        vertexCount,
    Vector3f&       center,
    float&          radius)
{
    if (vertexCount == 0)
    {
        center = Vector3f(0.0f, 0.0f, 0.0f);
        radius = 0.0f;
        return;
    }

    // 计算质心（近似包围球中心）
    Vector3f sum(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        const uint32_t idx = meshletVerts[i];
        sum.x += meshVertices[idx * 3 + 0];
        sum.y += meshVertices[idx * 3 + 1];
        sum.z += meshVertices[idx * 3 + 2];
    }
    center = sum / static_cast<float>(vertexCount);

    // 计算最大半径
    float maxDistSq = 0.0f;
    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        const uint32_t idx = meshletVerts[i];
        const float dx = meshVertices[idx * 3 + 0] - center.x;
        const float dy = meshVertices[idx * 3 + 1] - center.y;
        const float dz = meshVertices[idx * 3 + 2] - center.z;
        const float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > maxDistSq)
        {
            maxDistSq = distSq;
        }
    }
    radius = std::sqrt(maxDistSq);
}

NS_RENDERSYSTEM_END
