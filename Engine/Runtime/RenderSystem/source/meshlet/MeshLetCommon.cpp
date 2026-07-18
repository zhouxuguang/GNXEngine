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

	// 构建meshlet的包围盒
	std::vector<Vector4f> meshletBounds;
	for (auto& meshlet : meshlets)
	{
		auto bounds = meshopt_computeMeshletBounds(
			&meshletVertices[meshlet.vertex_offset],
			&meshletTriangles[meshlet.triangle_offset],
			meshlet.triangle_count,
            positions,
            vertexCount,
            positionStride);
		meshletBounds.push_back(Vector4f(bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius));
	}

    // 5. 将 meshopt 输出转换并重新打包到我们的 Meshlet 结构中
    outMeshlets.clear();
    outMeshlets.reserve(meshletCount);
    
    // Repack triangles from 3 consecutive byes to 4-byte uint32_t to
    // make it easier to unpack on the GPU.
    //
    std::vector<uint32_t> meshletTrianglesU32;
    for (size_t meshletIdx = 0; meshletIdx < meshlets.size(); meshletIdx++)
    {
        meshopt_Meshlet& m = meshlets[meshletIdx];
        // Save triangle offset for current meshlet
        uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());

        // Repack to uint32_t
        for (uint32_t triIdx = 0; triIdx < m.triangle_count; ++triIdx)
        {
            uint32_t i0 = 3 * triIdx + 0 + m.triangle_offset;
            uint32_t i1 = 3 * triIdx + 1 + m.triangle_offset;
            uint32_t i2 = 3 * triIdx + 2 + m.triangle_offset;

            uint8_t  vIdx0  = meshletTriangles[i0];
            uint8_t  vIdx1  = meshletTriangles[i1];
            uint8_t  vIdx2  = meshletTriangles[i2];
            uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
                              ((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
                              ((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
            meshletTrianglesU32.push_back(packed);
        }

        // 添加 meshlet — use meshletIdx, NOT i (which was shadowed by inner loop!)
        Meshlet meshlet = {};
        meshlet.vertexOffset   = m.vertex_offset;
        meshlet.triangleOffset = triangleOffset;
        meshlet.vertexCount    = m.vertex_count;
        meshlet.triangleCount  = m.triangle_count;
        meshlet.boundingSphere = meshletBounds[meshletIdx];  // FIXED: was using shadowed i
        outMeshlets.push_back(meshlet);
    }

    outVertices.swap(meshletVertices);
    outTriangles.swap(meshletTrianglesU32);
}

// =======================================================================
// SimplifyMesh
// =======================================================================

std::vector<uint32_t> SimplifyMesh(
    const uint32_t* indices,
    size_t          indexCount,
    const float*    positions,
    size_t          vertexCount,
    size_t          positionStride,
    size_t          targetIndexCount,
    float           targetError,
    uint32_t        options,
    float*          outResultError)
{
    // 结果数组必须 >= indexCount（原始索引数），因为 meshopt_simplify 需要工作缓冲区
    std::vector<uint32_t> result(indexCount);

    float error = 0.0f;
    size_t newCount = meshopt_simplify(
        result.data(),
        indices,
        indexCount,
        positions,
        vertexCount,
        positionStride,
        targetIndexCount,
        targetError,
        options,
        &error);

    if (outResultError)
        *outResultError = error;

    // 裁剪到实际简化后的索引数
    result.resize(newCount);
    return result;
}

NS_RENDERSYSTEM_END
