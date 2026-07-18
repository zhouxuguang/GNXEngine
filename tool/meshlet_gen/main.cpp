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
    // 3. Build meshlets using the engine's BuildMeshlets()
    // ===================================================================
    std::vector<Meshlet>   engineMeshlets;
    std::vector<uint32_t>  meshletVertices;
    std::vector<uint32_t>  packedTriangles;   // engine packs 3 uint8 -> 1 uint32

    BuildMeshlets(
        indices.data(), indexCount,
        positions.data(), vertexCount,
        sizeof(float) * 3,
        engineMeshlets,
        meshletVertices,
        packedTriangles);

    const size_t meshletCount = engineMeshlets.size();
    std::cout << "  Meshlets: " << meshletCount << std::endl;

    // ===================================================================
    // 3.5 测试 LOD 简化链：逐级 SimplifyMesh + BuildMeshlets
    // ===================================================================
    std::cout << "\n--- LOD Meshlet Chain Test ---" << std::endl;

    std::vector<uint32_t> lodIndices = indices;  // LOD 0 = 原始去重索引
    const size_t kMinTriangles = 256;             // 终止阈值
    int lodLevel = 0;

    while (lodIndices.size() / 3 > kMinTriangles)
    {
        size_t numTriangles = lodIndices.size() / 3;

        // 构建当前 LOD 的 meshlet
        std::vector<Meshlet>   lodMeshlets;
        std::vector<uint32_t>  lodMeshletVerts;
        std::vector<uint32_t>  lodMeshletTris;
        BuildMeshlets(lodIndices.data(), lodIndices.size(),
                      positions.data(), vertexCount, sizeof(float) * 3,
                      lodMeshlets, lodMeshletVerts, lodMeshletTris);

        std::cout << "LOD " << lodLevel << ": " << numTriangles << " triangles -> "
                  << lodMeshlets.size() << " meshlets" << std::endl;

        // 简化到 50%
        size_t targetCount = lodIndices.size() / 2;
        float error = 0.0f;
        auto simplified = SimplifyMesh(lodIndices.data(), lodIndices.size(),
                                       positions.data(), vertexCount, sizeof(float) * 3,
                                       targetCount,
                                       1.0f,       // targetError=1.0 表示不限制误差，完全靠 targetCount 控制
                                       0, &error);
        size_t simplifiedTris = simplified.size() / 3;
        std::cout << "  -> simplified to " << simplifiedTris << " triangles (error=" << error << ")" << std::endl;

        if (simplified.size() >= lodIndices.size())
        {
            std::cout << "  -> cannot simplify further, stopping" << std::endl;
            break;
        }

        lodIndices = std::move(simplified);
        ++lodLevel;
    }

    std::cout << "--- End LOD Test (" << lodLevel << " levels total) ---\n" << std::endl;

    // ===================================================================
    // 4. Serialize to binary (仅输出 LOD 0 的 meshlet)
    //    Engine Meshlet::triangleOffset is in uint32 units (packed 3 uint8
    //    per uint32), matching MeshLetFile.h reader. Output directly.
    // ===================================================================
    std::vector<uint8_t> bytes;

    // vertex count
    AppendToBytes(bytes, static_cast<uint32_t>(vertexCount));
    // vertex positions
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(positions.data()),
        reinterpret_cast<const uint8_t*>(positions.data()) + positions.size() * sizeof(float));
    // meshlet count
    AppendToBytes(bytes, static_cast<uint32_t>(meshletCount));
    // meshlet array (engine Meshlet, same layout as reader expects)
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(engineMeshlets.data()),
        reinterpret_cast<const uint8_t*>(engineMeshlets.data()) + engineMeshlets.size() * sizeof(Meshlet));
    // meshlet vertices count
    AppendToBytes(bytes, static_cast<uint32_t>(meshletVertices.size()));
    // meshlet vertices array
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(meshletVertices.data()),
        reinterpret_cast<const uint8_t*>(meshletVertices.data()) + meshletVertices.size() * sizeof(uint32_t));
    // meshlet triangles count (in uint32_t units)
    AppendToBytes(bytes, static_cast<uint32_t>(packedTriangles.size()));
    // meshlet triangles array (uint32_t[], packed)
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(packedTriangles.data()),
        reinterpret_cast<const uint8_t*>(packedTriangles.data()) + packedTriangles.size() * sizeof(uint32_t));

    // ===================================================================
    // 5. Write to file using engine's FileUtil
    // ===================================================================
    if (!baselib::FileUtil::WriteBinaryFile(outputFile, bytes))
        return 1;

    std::cout << "Successfully wrote " << bytes.size() << " bytes to " << outputFile << std::endl;

    std::cout << "\nMeshlet details:" << std::endl;
    for (size_t i = 0; i < meshletCount; ++i)
    {
        const auto& m = engineMeshlets[i];
        std::cout << "  Meshlet[" << i << "]: vertices=" << m.vertexCount
                  << ", triangles=" << m.triangleCount
                  << ", vertex_offset=" << m.vertexOffset
                  << ", triangle_offset=" << m.triangleOffset
                  << std::endl;
    }

    return 0;
}
