//
//  MeshLetFile.h
//  GNXEngine
//
//  Meshlet binary file reader utility.
//
//  用于读取 meshlet_gen 工具生成的二进制 meshlet 文件，
//  运行时直接加载即可使用。
//
//  文件格式（小端序）：
//  1. uint32_t       vertexCount             顶点个数
//  2. float[vc][3]   vertexPositions         顶点位置数据 (vc = vertexCount)
//  3. uint32_t       meshletCount            meshlet 个数
//  4. meshopt_Meshlet[meshletCount]          meshlet 数组
//  5. uint32_t       meshletVerticesCount    meshlet 顶点索引个数
//  6. uint32_t[mvc]  meshletVertices         meshlet 顶点索引数组 (mvc = meshletVerticesCount)
//  7. uint32_t       meshletTrianglesCount   meshlet 三角形索引个数
//  8. uint32_t[mtc]   meshletTriangles        packed triangle index array (mtc = meshletTrianglesCount, each uint32_t packs 3 uint8_t indices, same format as MeshLetCommon.h output)
//

#ifndef GNXENGINE_MESHLET_FILE_INCLUDE_H
#define GNXENGINE_MESHLET_FILE_INCLUDE_H

#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <cassert>

#include "MeshLetCommon.h"

NS_RENDERSYSTEM_BEGIN

// -----------------------------------------------------------------------
// MeshletFileData
// -----------------------------------------------------------------------
// 从 meshlet 二进制文件中加载的完整数据
struct MeshletFileData
{
    // 顶点位置 (float[vertexCount][3])
    std::vector<float>    vertexPositions;

    // meshlet 数组
    std::vector<Meshlet> meshlets;

    // meshlet 顶点索引 (meshlet i 的顶点索引范围在
    // [meshlets[i].vertex_offset, meshlets[i].vertex_offset + meshlets[i].vertex_count))
    std::vector<uint32_t> meshletVertices;

    // meshlet 三角形索引 (打包为 uint32_t[]，每个 uint32_t 存 3 个 uint8_t 索引)
    // meshlet i 的三角形索引范围在 [meshlets[i].triangle_offset, meshlets[i].triangle_offset + meshlets[i].triangle_count)
    // 每个 triangle_offset 以 uint32_t 为单位，一个三角形对应 3 个 uint8_t 索引 = 1 个 uint32_t packed
    std::vector<uint32_t>  meshletTriangles;

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertexPositions.size() / 3); }
    uint32_t GetMeshletCount() const { return static_cast<uint32_t>(meshlets.size()); }
};

// -----------------------------------------------------------------------
// LoadMeshletFile
// -----------------------------------------------------------------------
// 从 meshlet_gen 工具生成的二进制文件中加载 meshlet 数据。
// 参数:
//   filePath   - 二进制文件路径
//   outData    - [输出] 加载后的 meshlet 数据
// 返回: true 表示加载成功
inline bool LoadMeshletFile(const std::string& filePath, MeshletFileData& outData)
{
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp)
    {
        return false;
    }

    // 读取整个文件到内存
    fseek(fp, 0, SEEK_END);
    const size_t fileSize = static_cast<size_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);

    std::vector<uint8_t> buffer(fileSize);
    if (fread(buffer.data(), 1, fileSize, fp) != fileSize)
    {
        fclose(fp);
        return false;
    }
    fclose(fp);

    // 解析
    size_t offset = 0;

    // 1. 顶点个数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t vertexCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 2. 顶点位置数据 (float[vertexCount][3])
    const size_t posBytes = static_cast<size_t>(vertexCount) * 3 * sizeof(float);
    if (offset + posBytes > fileSize) return false;
    outData.vertexPositions.resize(static_cast<size_t>(vertexCount) * 3);
    std::memcpy(outData.vertexPositions.data(), buffer.data() + offset, posBytes);
    offset += posBytes;

    // 3. meshlet 个数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 4. meshlet 数组 (meshopt_Meshlet[meshletCount])
    const size_t meshletBytes = static_cast<size_t>(meshletCount) * sizeof(Meshlet);
    if (offset + meshletBytes > fileSize) return false;
    outData.meshlets.resize(meshletCount);
    std::memcpy(outData.meshlets.data(), buffer.data() + offset, meshletBytes);
    offset += meshletBytes;

    // 5. meshletVertices 个数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletVerticesCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 6. meshletVertices 数组 (uint32_t[meshletVerticesCount])
    const size_t mvBytes = static_cast<size_t>(meshletVerticesCount) * sizeof(uint32_t);
    if (offset + mvBytes > fileSize) return false;
    outData.meshletVertices.resize(meshletVerticesCount);
    std::memcpy(outData.meshletVertices.data(), buffer.data() + offset, mvBytes);
    offset += mvBytes;

    // 7. meshletTriangles 个数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletTrianglesCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 8. meshletTriangles 数组 (uint32_t[meshletTrianglesCount])
    const size_t mtBytes = static_cast<size_t>(meshletTrianglesCount) * sizeof(uint32_t);
    if (offset + mtBytes > fileSize) return false;
    outData.meshletTriangles.resize(meshletTrianglesCount);
    std::memcpy(outData.meshletTriangles.data(), buffer.data() + offset, mtBytes);
    offset += mtBytes;

    assert(offset == fileSize);
    return true;
}

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MESHLET_FILE_INCLUDE_H
