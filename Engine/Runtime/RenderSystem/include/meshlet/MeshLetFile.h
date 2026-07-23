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
//  [Header]
//  1. uint32_t       vertexCount             顶点个数（所有 LOD 共享）
//  2. float[vc][3]   vertexPositions         顶点位置数据
//  3. uint32_t       lodCount                LOD 数量
//  [LOD 数组]
//  4. uint32_t[lodCount] lodMeshletOffsets   每个 LOD 第一个 meshlet 的索引
//  5. uint32_t[lodCount] lodMeshletCounts    每个 LOD 的 meshlet 数量
//  [Meshlet 数据]
//  6. uint32_t       totalMeshletCount       所有 LOD 的 meshlet 总数
//  7. Meshlet[totalMeshletCount]             meshlet 平坦数组
//  8. uint32_t       totalVerticesCount      meshlet 顶点索引总数
//  9. uint32_t[tvc]  meshletVertices         顶点索引平坦数组
//  10. uint32_t      totalTrianglesCount     三角形索引总数
//  11. uint32_t[ttc] meshletTriangles        打包三角形索引平坦数组
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
    // 顶点位置 (float[vertexCount][3]) — 所有 LOD 共享
    std::vector<float>    vertexPositions;

    // 所有 LOD 的 meshlet 平坦数组
    std::vector<Meshlet> meshlets;

    // 所有 LOD 的 meshlet 顶点索引
    std::vector<uint32_t> meshletVertices;

    // 所有 LOD 的 meshlet 三角形索引 (packed)
    std::vector<uint32_t>  meshletTriangles;

    // LOD 信息
    uint32_t              lodCount = 0;
    std::vector<uint32_t> lodMeshletOffsets;   // [lodCount]
    std::vector<uint32_t> lodMeshletCounts;    // [lodCount]

    // METIS 分区结果: part[i] ∈ [0, numPartitions-1]
    uint32_t              numPartitions = 0;
    std::vector<uint32_t> meshletPartitions;   // [meshletCount]

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertexPositions.size() / 3); }
    uint32_t GetMeshletCount() const { return static_cast<uint32_t>(meshlets.size()); }
    uint32_t GetLODCount() const { return lodCount; }
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

    // 3. LOD 数量
    if (offset + sizeof(uint32_t) > fileSize) return false;
    outData.lodCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 4. LOD meshlet offsets
    {
        const size_t lodBytes = static_cast<size_t>(outData.lodCount) * sizeof(uint32_t);
        if (offset + lodBytes > fileSize) return false;
        outData.lodMeshletOffsets.resize(outData.lodCount);
        std::memcpy(outData.lodMeshletOffsets.data(), buffer.data() + offset, lodBytes);
        offset += lodBytes;
    }

    // 5. LOD meshlet counts
    {
        const size_t lodBytes = static_cast<size_t>(outData.lodCount) * sizeof(uint32_t);
        if (offset + lodBytes > fileSize) return false;
        outData.lodMeshletCounts.resize(outData.lodCount);
        std::memcpy(outData.lodMeshletCounts.data(), buffer.data() + offset, lodBytes);
        offset += lodBytes;
    }

    // 6. meshlet 总数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 7. meshlet 平坦数组 (Meshlet[meshletCount])
    const size_t meshletBytes = static_cast<size_t>(meshletCount) * sizeof(Meshlet);
    if (offset + meshletBytes > fileSize) return false;
    outData.meshlets.resize(meshletCount);
    std::memcpy(outData.meshlets.data(), buffer.data() + offset, meshletBytes);
    offset += meshletBytes;

    // 8. meshletVertices 总数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletVerticesCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 9. meshletVertices 平坦数组 (uint32_t[meshletVerticesCount])
    const size_t mvBytes = static_cast<size_t>(meshletVerticesCount) * sizeof(uint32_t);
    if (offset + mvBytes > fileSize) return false;
    outData.meshletVertices.resize(meshletVerticesCount);
    std::memcpy(outData.meshletVertices.data(), buffer.data() + offset, mvBytes);
    offset += mvBytes;

    // 10. meshletTriangles 总数
    if (offset + sizeof(uint32_t) > fileSize) return false;
    const uint32_t meshletTrianglesCount = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 11. meshletTriangles 平坦数组 (uint32_t[meshletTrianglesCount])
    const size_t mtBytes = static_cast<size_t>(meshletTrianglesCount) * sizeof(uint32_t);
    if (offset + mtBytes > fileSize) return false;
    outData.meshletTriangles.resize(meshletTrianglesCount);
    std::memcpy(outData.meshletTriangles.data(), buffer.data() + offset, mtBytes);
    offset += mtBytes;

    // 12. numPartitions
    if (offset + sizeof(uint32_t) > fileSize) return false;
    outData.numPartitions = *reinterpret_cast<const uint32_t*>(buffer.data() + offset);
    offset += sizeof(uint32_t);

    // 13. meshletPartitions (uint32_t[meshletCount])
    if (outData.numPartitions > 0)
    {
        const size_t partBytes = static_cast<size_t>(meshletCount) * sizeof(uint32_t);
        if (offset + partBytes > fileSize) return false;
        outData.meshletPartitions.resize(meshletCount);
        std::memcpy(outData.meshletPartitions.data(), buffer.data() + offset, partBytes);
        offset += partBytes;
    }

    assert(offset == fileSize);
    return true;
}

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MESHLET_FILE_INCLUDE_H
