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
#include "Runtime/RenderSystem/include/meshlet/MeshLetBuilder.h"
#include "Runtime/RenderSystem/include/meshlet/MeshLetFile.h"
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

    std::vector<uint32_t> remap(vertexCount);
    const size_t uniqueCount = meshopt_generateVertexRemap(
        remap.data(),
        expandedIndices.data(), expandedIndices.size(),
        expandedPositions.data(), vertexCount,
        sizeof(float) * 3);

    outIndices.resize(expandedIndices.size());
    meshopt_remapIndexBuffer(outIndices.data(), expandedIndices.data(),
                             expandedIndices.size(), remap.data());

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

    // ===================================================================
    // 3. Deduplicate to unique positions only
    // ===================================================================
    std::vector<float>    dedupPositions;
    std::vector<uint32_t> dedupIndices;
    DeduplicatePositions(expandedPositions, expandedIndices, dedupPositions, dedupIndices);

    const uint32_t dedupVertexCount = static_cast<uint32_t>(dedupPositions.size() / 3);
    std::cout << "  Unique positions (after dedup): " << dedupVertexCount << std::endl;
    std::cout << "  Index count:                    " << dedupIndices.size() << std::endl;

    // ===================================================================
    // 4. 使用 MeshletBuilder 构建 meshlet（含 METIS 连通图分区）
    // ===================================================================
    std::cout << "\n--- Building Meshlets with MeshletBuilder ---" << std::endl;

    MeshletBuilder builder;
    builder.SetNumPartitions(2);  // 分为两组，便于调试验证
    MeshletFileData outData;
    if (!builder.Build(dedupPositions.data(), dedupVertexCount,
                       dedupIndices.data(), dedupIndices.size(),
                       outData))
    {
        std::cerr << "Error: MeshletBuilder::Build failed." << std::endl;
        return 1;
    }

    const uint32_t vertexCount = static_cast<uint32_t>(outData.vertexPositions.size() / 3);
    std::cout << "  Vertex count:   " << vertexCount << std::endl;
    std::cout << "  Meshlet count:  " << outData.meshlets.size() << std::endl;
    std::cout << "  LOD count:      " << outData.lodCount << std::endl;

    // ===================================================================
    // 5. 测试合并：将同组 meshlet 合并为一个网格，然后简化
    // ===================================================================
    if (outData.numPartitions > 0)
    {
        std::cout << "\n--- Merging meshlet groups ---" << std::endl;

        std::vector<MergedGroup> mergedGroups;
        builder.MergeGroups(outData, mergedGroups);

        // ---- 对每组合并网格做简化（锁定组边界） ----
        std::cout << "\n--- Simplifying merged groups (lock border) ---" << std::endl;
        for (size_t g = 0; g < mergedGroups.size(); ++g)
        {
            MergedGroup& group = mergedGroups[g];
            const uint32_t triCount = static_cast<uint32_t>(group.triangleIndices.size() / 3);
            if (triCount == 0) continue;

            // 一行封装：compact → meshopt_simplify(LockBorder) → map back
            size_t targetIdxCount = group.triangleIndices.size() / 2;  // 保留 50%
            float error = 0.0f;
            std::vector<uint32_t> simplifiedGlobal = group.Simplify(
                outData.vertexPositions.data(),
                outData.GetVertexCount(),
                targetIdxCount,
                0.01f,
                &error);

            std::cout << "  group " << g << ": "
                      << triCount << " tri"
                      << " -> " << simplifiedGlobal.size() / 3 << " tri"
                      << " (keep " << (100.0 * simplifiedGlobal.size() / group.triangleIndices.size()) << "%)"
                      << ", error=" << error
                      << std::endl;
        }
    }

    // ===================================================================
    // 6. Serialize to binary
    // ===================================================================
    std::vector<uint8_t> bytes;

    // ---- Header ----
    AppendToBytes(bytes, vertexCount);
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.vertexPositions.data()),
        reinterpret_cast<const uint8_t*>(outData.vertexPositions.data())
            + outData.vertexPositions.size() * sizeof(float));

    AppendToBytes(bytes, outData.lodCount);

    // ---- LOD 数组 ----
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.lodMeshletOffsets.data()),
        reinterpret_cast<const uint8_t*>(outData.lodMeshletOffsets.data())
            + outData.lodMeshletOffsets.size() * sizeof(uint32_t));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.lodMeshletCounts.data()),
        reinterpret_cast<const uint8_t*>(outData.lodMeshletCounts.data())
            + outData.lodMeshletCounts.size() * sizeof(uint32_t));

    // ---- Meshlet 平坦数组 ----
    AppendToBytes(bytes, static_cast<uint32_t>(outData.meshlets.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.meshlets.data()),
        reinterpret_cast<const uint8_t*>(outData.meshlets.data())
            + outData.meshlets.size() * sizeof(Meshlet));

    AppendToBytes(bytes, static_cast<uint32_t>(outData.meshletVertices.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.meshletVertices.data()),
        reinterpret_cast<const uint8_t*>(outData.meshletVertices.data())
            + outData.meshletVertices.size() * sizeof(uint32_t));

    AppendToBytes(bytes, static_cast<uint32_t>(outData.meshletTriangles.size()));
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outData.meshletTriangles.data()),
        reinterpret_cast<const uint8_t*>(outData.meshletTriangles.data())
            + outData.meshletTriangles.size() * sizeof(uint32_t));

    // ---- METIS 分区数据 ----
    AppendToBytes(bytes, outData.numPartitions);
    if (outData.numPartitions > 0)
    {
        bytes.insert(bytes.end(),
            reinterpret_cast<const uint8_t*>(outData.meshletPartitions.data()),
            reinterpret_cast<const uint8_t*>(outData.meshletPartitions.data())
                + outData.meshletPartitions.size() * sizeof(uint32_t));
    }

    // ===================================================================
    // 6. Write to file
    // ===================================================================
    if (!baselib::FileUtil::WriteBinaryFile(outputFile, bytes))
        return 1;

    std::cout << "Successfully wrote " << bytes.size() << " bytes to " << outputFile << std::endl;

    return 0;
}
