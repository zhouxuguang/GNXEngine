//
//  main.cpp
//  meshlet_gen
//
//  Meshlet Generation Tool
//
//  Loads a 3D mesh via the engine's MeshImporter (assimp-based),
//  deduplicates vertex positions to match unique position count (like MeshLab),
//  builds meshlets, and writes the result to a binary file.
//
//  Output binary format (little-endian):
//  1. uint32_t       vertexCount             number of vertices
//  2. float[vc][3]   vertexPositions         vertex position data (vc = vertexCount)
//  3. uint32_t       meshletCount            number of meshlets
//  4. meshopt_Meshlet[meshletCount]          meshlet array
//  5. uint32_t       meshletVerticesCount    number of meshlet vertex indices
//  6. uint32_t[mvc]  meshletVertices         meshlet vertex index array (mvc = meshletVerticesCount)
//  7. uint32_t       meshletTrianglesCount   number of packed meshlet triangle indices (in uint32_t units)
//  8. uint32_t[mtc]  meshletTriangles        packed triangle index array (mtc = meshletTrianglesCount, each uint32_t packs 3 uint8_t indices)
//

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#include "Runtime/RenderSystem/include/meshlet/MeshLetCommon.h"
#include "Runtime/RenderSystem/include/mesh/MeshImporter.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"
#include "Runtime/BaseLib/include/FileUtil.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "meshoptimizer.h"

using namespace RenderSystem;
using namespace mathutil;

// -----------------------------------------------------------------------
// Print usage information
// -----------------------------------------------------------------------
static void PrintUsage(const char* appName)
{
    std::cerr << "Usage: " << appName << " <input_mesh> <output_file>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  Reads a mesh from <input_mesh> (obj/fbx/gltf/glb) using the" << std::endl;
    std::cerr << "  engine's MeshImporter, deduplicates positions, builds" << std::endl;
    std::cerr << "  meshlets, and writes them to <output_file>." << std::endl;
}

// -----------------------------------------------------------------------
// Helper: append a single value to a byte vector
// -----------------------------------------------------------------------
template<typename T>
static void AppendToBytes(std::vector<uint8_t>& bytes, const T& value)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
    bytes.insert(bytes.end(), p, p + sizeof(T));
}

// -----------------------------------------------------------------------
// Deduplicate vertex positions using meshoptimizer, rebuild indices.
// The engine's MeshImporter expands vertices per (pos+normal+uv+tangent),
// but we only need unique vertex positions for meshlet building.
// -----------------------------------------------------------------------
static void DeduplicatePositions(
    const std::vector<float>& expandedPositions,
    const std::vector<uint32_t>& expandedIndices,
    std::vector<float>& outPositions,
    std::vector<uint32_t>& outIndices)
{
    const size_t vertexCount = expandedPositions.size() / 3;

    // meshopt_generateVertexRemap deduplicates vertices with identical data.
    // We pass only the position field (float3, stride=12) for comparison,
    // so vertices with the same position (but different normal/uv) merge.
    std::vector<uint32_t> remap(vertexCount);
    const size_t uniqueCount = meshopt_generateVertexRemap(
        remap.data(),
        expandedIndices.data(), expandedIndices.size(),
        expandedPositions.data(), vertexCount,
        sizeof(float) * 3);  // stride: only compare float3 positions

    // Remap indices: expanded index → unique index
    outIndices.resize(expandedIndices.size());
    meshopt_remapIndexBuffer(outIndices.data(), expandedIndices.data(),
                             expandedIndices.size(), remap.data());

    // Compact the position buffer to unique vertices only
    outPositions.resize(uniqueCount * 3);
    meshopt_remapVertexBuffer(outPositions.data(), expandedPositions.data(),
                              vertexCount, sizeof(float) * 3, remap.data());
}

// -----------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string inputFile  = argv[1];
    const std::string outputFile = argv[2];

    std::cout << "Input mesh:  " << inputFile << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;

    // ===================================================================
    // 1. Load mesh via engine's MeshImporter (assimp-based)
    // ===================================================================
    MeshImporter* importer = CreateMeshImporter();
    if (!importer)
    {
        std::cerr << "Error: Failed to create mesh importer." << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!importer->ImportFromFile(inputFile, &mesh, nullptr))
    {
        std::cerr << "Error: Failed to load mesh file: " << inputFile << std::endl;
        DestroyMeshImporter(importer);
        return 1;
    }
    //DestroyMeshImporter(importer);

    // ===================================================================
    // 2. Extract expanded vertex positions and indices
    // ===================================================================
    const uint32_t expandedVertexCount = mesh.GetVertexCount();
    const auto& expandedIndices = mesh.GetIndices();
    const size_t indexCount = expandedIndices.size();

    if (expandedVertexCount == 0 || indexCount == 0)
    {
        std::cerr << "Error: Mesh has no vertices or indices." << std::endl;
        return 1;
    }

    // Copy expanded positions from the interleaved VertexData
    std::vector<float> expandedPositions;
    expandedPositions.reserve(static_cast<size_t>(expandedVertexCount) * 3);
    {
        auto posIt = mesh.GetPositionBegin();
        for (uint32_t i = 0; i < expandedVertexCount; ++i, ++posIt)
        {
            expandedPositions.push_back(posIt->x);
            expandedPositions.push_back(posIt->y);
            expandedPositions.push_back(posIt->z);
        }
    }

    std::cout << "  Expanded vertices (from engine MeshImporter): " << expandedVertexCount << std::endl;

    // Deduplicate to unique positions only (matching MeshLab count)
    std::vector<float>    positions;
    std::vector<uint32_t> indices;
    DeduplicatePositions(expandedPositions, expandedIndices, positions, indices);

    const uint32_t vertexCount = static_cast<uint32_t>(positions.size() / 3);
    std::cout << "  Unique positions (after dedup, like MeshLab): " << vertexCount << std::endl;
    std::cout << "  Indices:                                     " << indices.size() << std::endl;

    // ===================================================================
    // 3. 构建所有 LOD 的 meshlet（平坦合并数组）
    //    参考: chaoticbob mesh-shading-part-5, 所有 LOD 合并到一个数组
    // ===================================================================
    std::cout << "\n--- Building LOD Meshlets ---" << std::endl;

    std::vector<float>    combinedPositions;      // 顶点位置（所有 LOD 共享同一份）
    std::vector<Meshlet>  combinedMeshlets;        // 所有 LOD 的 meshlet 平坦数组
    std::vector<uint32_t> combinedVertexIndices;   // 所有 LOD 的顶点索引平坦数组
    std::vector<uint32_t> combinedTriangleIndices; // 所有 LOD 的三角形索引平坦数组
    std::vector<uint32_t> lodMeshletOffsets;       // [lodCount] 每个 LOD 第一个 meshlet 索引
    std::vector<uint32_t> lodMeshletCounts;        // [lodCount] 每个 LOD meshlet 数量

    // 顶点位置是所有 LOD 共享的（meshopt_simplify 返回的索引指向原始顶点）
    combinedPositions = positions;

    std::vector<uint32_t> lodIndices = indices;
    const size_t kMinTriangles = 256;
    const int    kMaxLodLevels = 5;
    int lodLevel = 0;

    while ((lodIndices.size() / 3 > kMinTriangles) && (lodLevel < kMaxLodLevels))
    {
        size_t numTriangles = lodIndices.size() / 3;

        // 构建当前 LOD 的 meshlet
        std::vector<Meshlet>   lodMeshlets;
        std::vector<uint32_t>  lodMeshletVerts;
        std::vector<uint32_t>  lodMeshletTris;
        BuildMeshlets(lodIndices.data(), lodIndices.size(),
                      combinedPositions.data(), vertexCount, sizeof(float) * 3,
                      lodMeshlets, lodMeshletVerts, lodMeshletTris);

        // 记录当前 LOD 的偏移和数量
        lodMeshletOffsets.push_back(static_cast<uint32_t>(combinedMeshlets.size()));
        lodMeshletCounts.push_back(static_cast<uint32_t>(lodMeshlets.size()));

        // 记录偏移量用于调整 meshlet 数据
        uint32_t vertexIndexBase = static_cast<uint32_t>(combinedVertexIndices.size());
        uint32_t triangleBase    = static_cast<uint32_t>(combinedTriangleIndices.size());

        // 调整 meshlet 的偏移量到合并数组中的位置
        for (auto& m : lodMeshlets)
        {
            m.vertexOffset   += vertexIndexBase;
            m.triangleOffset += triangleBase;
            combinedMeshlets.push_back(m);
        }

        // 合并顶点索引和三角形索引（不需要偏移调整，因为指向的是原始顶点数组）
        combinedVertexIndices.insert(combinedVertexIndices.end(),
            lodMeshletVerts.begin(), lodMeshletVerts.end());
        combinedTriangleIndices.insert(combinedTriangleIndices.end(),
            lodMeshletTris.begin(), lodMeshletTris.end());

        std::cout << "LOD " << lodLevel << ": " << numTriangles << " triangles -> "
                  << lodMeshlets.size() << " meshlets "
                  << "(offset=" << lodMeshletOffsets.back()
                  << ", count=" << lodMeshletCounts.back() << ")" << std::endl;

        // 简化到 50% 进入下一级
        size_t targetCount = lodIndices.size() / 2;
        float error = 0.0f;
        auto simplified = SimplifyMesh(lodIndices.data(), lodIndices.size(),
                                       combinedPositions.data(), vertexCount, sizeof(float) * 3,
                                       targetCount, 1.0f, 0, &error);
        if (simplified.size() >= lodIndices.size())
        {
            std::cout << "  -> cannot simplify further, stopping" << std::endl;
            break;
        }
        std::cout << "  -> simplified to " << (simplified.size() / 3)
                  << " triangles (error=" << error << ")" << std::endl;

        lodIndices = std::move(simplified);
        ++lodLevel;
    }

    const uint32_t totalLodCount = static_cast<uint32_t>(lodMeshletOffsets.size());
    std::cout << "--- Total: " << totalLodCount << " LODs, "
              << combinedMeshlets.size() << " total meshlets ---\n" << std::endl;

    // ===================================================================
    // 4. Serialize to binary (多 LOD 格式)
    // ===================================================================
    std::vector<uint8_t> bytes;

    // ---- Header ----
    AppendToBytes(bytes, static_cast<uint32_t>(vertexCount));         // vertex count

    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(combinedPositions.data()),
        reinterpret_cast<const uint8_t*>(combinedPositions.data())
            + combinedPositions.size() * sizeof(float));               // vertex positions

    AppendToBytes(bytes, totalLodCount);                               // lod count

    // ---- LOD 数组 ----
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(lodMeshletOffsets.data()),
        reinterpret_cast<const uint8_t*>(lodMeshletOffsets.data())
            + lodMeshletOffsets.size() * sizeof(uint32_t));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(lodMeshletCounts.data()),
        reinterpret_cast<const uint8_t*>(lodMeshletCounts.data())
            + lodMeshletCounts.size() * sizeof(uint32_t));

    // ---- Meshlet 平坦数组 ----
    AppendToBytes(bytes, static_cast<uint32_t>(combinedMeshlets.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(combinedMeshlets.data()),
        reinterpret_cast<const uint8_t*>(combinedMeshlets.data())
            + combinedMeshlets.size() * sizeof(Meshlet));

    AppendToBytes(bytes, static_cast<uint32_t>(combinedVertexIndices.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(combinedVertexIndices.data()),
        reinterpret_cast<const uint8_t*>(combinedVertexIndices.data())
            + combinedVertexIndices.size() * sizeof(uint32_t));

    AppendToBytes(bytes, static_cast<uint32_t>(combinedTriangleIndices.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(combinedTriangleIndices.data()),
        reinterpret_cast<const uint8_t*>(combinedTriangleIndices.data())
            + combinedTriangleIndices.size() * sizeof(uint32_t));

    // ===================================================================
    // 5. Write to file using engine's FileUtil
    // ===================================================================
    if (!baselib::FileUtil::WriteBinaryFile(outputFile, bytes))
        return 1;

    std::cout << "Successfully wrote " << bytes.size() << " bytes to " << outputFile << std::endl;

    std::cout << "\nLOD Meshlet summary:" << std::endl;
    for (uint32_t lod = 0; lod < totalLodCount; ++lod)
    {
        uint32_t first = lodMeshletOffsets[lod];
        uint32_t count = lodMeshletCounts[lod];
        std::cout << "  LOD " << lod << ": " << count << " meshlets "
                  << "(offset=" << first << ")" << std::endl;
    }

    return 0;
}
