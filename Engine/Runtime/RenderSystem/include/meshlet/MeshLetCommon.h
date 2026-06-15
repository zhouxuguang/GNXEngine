//
//  MeshLetCommon.h
//  GNXEngine
//
//  Created by zhouxuguang on 2024/6/14.
//

#ifndef GNXENGINE_MESHLET_COMMON_INCLUDE_H
#define GNXENGINE_MESHLET_COMMON_INCLUDE_H

#include "../RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"

NS_RENDERSYSTEM_BEGIN

// -----------------------------------------------------------------------
// Meshlet 常量
// -----------------------------------------------------------------------

// NVIDIA 建议的通用最大值：126 三角形 / 64 顶点
// meshopt 要求三角形数按 4 对齐，因此使用 124
static constexpr uint32_t kMeshletMaxVertices  = 64;
static constexpr uint32_t kMeshletMaxTriangles = 124;

// Mesh Shader 每个线程组的线程数（足够覆盖最大三角形数）
static constexpr uint32_t kMeshletThreadGroupSize = 128;

// -----------------------------------------------------------------------
// Meshlet（CPU 端，用于构建和上传到 GPU）
// -----------------------------------------------------------------------

/**
 * Meshlet 基本结构
 *
 * 与 meshopt_Meshlet 对齐，用于存储 meshlet 的顶点/三角形偏移和计数。
 * 每个 meshlet 是原始网格的一个子集：
 *   - 顶点索引指向 meshletVerticesBuffer（local → global 映射）
 *   - 三角形索引指向 meshletTrianglesBuffer（打包为 uint32_t）
 */
struct Meshlet
{
    uint32_t vertexOffset;      // meshletVerticesBuffer 中的偏移量
    uint32_t triangleOffset;    // meshletTrianglesBuffer 中的偏移量（以 uint32_t 为单位，每个 uint32_t 打包了 3 个 uint8_t 索引）
    uint32_t vertexCount;       // 本 meshlet 使用的顶点数（≤ kMeshletMaxVertices）
    uint32_t triangleCount;     // 本 meshlet 使用的三角形数（≤ kMeshletMaxTriangles）
};

// -----------------------------------------------------------------------
// 顶点结构（Mesh Shader 输入）
// -----------------------------------------------------------------------

/**
 * meshlet 中使用的顶点数据（位置）
 * 使用 packed_float3 风格以紧密打包，与 Metal 的 packed_float3 对齐
 */
struct MeshletVertex
{
    float position[3];          // 位置（12 字节，紧密对齐）
};

// -----------------------------------------------------------------------
// Meshlet 构建辅助函数
// -----------------------------------------------------------------------

/**
 * 使用 meshopt 从给定的索引和位置数据构建 meshlet
 *
 * @param indices        网格三角形索引
 * @param indexCount     索引数量
 * @param positions      顶点位置
 * @param vertexCount    顶点数量
 * @param positionStride 位置步长（字节）
 * @param outMeshlets    输出的 meshlet 数组
 * @param outVertices    输出的 meshlet 顶点索引映射
 * @param outTriangles   输出的重新打包后的三角形索引（uint32_t）
 */
RENDERSYSTEM_API void BuildMeshlets(
    const uint32_t* indices,
    size_t          indexCount,
    const float*    positions,
    size_t          vertexCount,
    size_t          positionStride,
    std::vector<Meshlet>&           outMeshlets,
    std::vector<uint32_t>&          outVertices,
    std::vector<uint32_t>&          outTriangles);

/**
 * 计算 meshlet 的包围球（用于视锥剔除）
 *
 * @param meshVertices   原始网格顶点位置
 * @param meshletVerts   meshlet 的顶点索引映射
 * @param vertexCount    meshlet 顶点数量
 * @param center         [输出] 包围球中心
 * @param radius         [输出] 包围球半径
 */
RENDERSYSTEM_API void ComputeMeshletBounds(
    const float*    meshVertices,
    const uint32_t* meshletVerts,
    uint32_t        vertexCount,
    mathutil::Vector3f&       center,
    float&          radius);

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MESHLET_COMMON_INCLUDE_H
