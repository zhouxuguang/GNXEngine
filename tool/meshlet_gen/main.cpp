//
//  main.cpp
//  meshlet_gen
//
//  Meshlet Generation Tool
//
//  Loads a 3D mesh via assimp (same library & settings as the engine),
//  uses the engine's BuildMeshlets() to generate meshlets,
//  and writes the result to a binary file.
//
//  Output binary format (little-endian):
//  1. uint32_t       vertexCount             number of vertices
//  2. float[vc][3]   vertexPositions         vertex position data (vc = vertexCount)
//  3. uint32_t       meshletCount            number of meshlets
//  4. meshopt_Meshlet[meshletCount]          meshlet array
//  5. uint32_t       meshletVerticesCount    number of meshlet vertex indices
//  6. uint32_t[mvc]  meshletVertices         meshlet vertex index array (mvc = meshletVerticesCount)
//  7. uint32_t       meshletTrianglesCount   number of packed meshlet triangle indices (in uint32_t units)
//  8. uint32_t[mtc]   meshletTriangles        packed triangle index array (mtc = meshletTrianglesCount, each uint32_t packs 3 uint8_t indices)
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
    std::cerr << "  Reads a mesh from <input_mesh> (obj/fbx/gltf/glb) using the engine's" << std::endl;
    std::cerr << "  built-in MeshImporter (assimp-based), builds meshlets, and writes" << std::endl;
    std::cerr << "  them to <output_file>." << std::endl;
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
// Unpack a packed uint32_t triangle to 3 uint8_t values
// -----------------------------------------------------------------------
static void UnpackTriangle(uint32_t packed, uint8_t& a, uint8_t& b, uint8_t& c)
{
    a = static_cast<uint8_t>((packed >> 0)  & 0xFF);
    b = static_cast<uint8_t>((packed >> 8)  & 0xFF);
    c = static_cast<uint8_t>((packed >> 16) & 0xFF);
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
    // 2. Extract vertex positions and indices from the engine Mesh
    // ===================================================================
    const uint32_t vertexCount = mesh.GetVertexCount();
    const auto& indices = mesh.GetIndices();
    const size_t indexCount = indices.size();

    if (vertexCount == 0 || indexCount == 0)
    {
        std::cerr << "Error: Mesh has no vertices or indices." << std::endl;
        return 1;
    }

    // Copy positions from the interleaved VertexData to a contiguous float array
    std::vector<float> positions;
    positions.reserve(static_cast<size_t>(vertexCount) * 3);
    {
        auto posIt = mesh.GetPositionBegin();
        for (uint32_t i = 0; i < vertexCount; ++i, ++posIt)
        {
            positions.push_back(posIt->x);
            positions.push_back(posIt->y);
            positions.push_back(posIt->z);
        }
    }

    std::cout << "  Vertices: " << vertexCount << std::endl;
    std::cout << "  Indices:  " << indexCount << std::endl;

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
    // 4. Convert engine output -> output format
    //    Engine Meshlet::triangleOffset is in uint32 units, and the
    //    triangles are already packed as uint32_t (3 uint8 indices per
    //    uint32_t). We store them directly (no unpack) so the GPU can
    //    read byte-level data from a uint32_t[] SSBO.
    // ===================================================================
    std::vector<meshopt_Meshlet> outMeshlets(meshletCount);
    std::vector<uint32_t>        outTriangles;

    for (size_t i = 0; i < meshletCount; ++i)
    {
        const Meshlet& src = engineMeshlets[i];
        meshopt_Meshlet& dst = outMeshlets[i];

        dst.vertex_offset   = src.vertexOffset;
        dst.triangle_offset = src.triangleOffset;
        dst.vertex_count    = src.vertexCount;
        dst.triangle_count  = src.triangleCount;

        for (uint32_t t = 0; t < src.triangleCount; ++t)
        {
            outTriangles.push_back(packedTriangles[src.triangleOffset + t]);
        }
    }

    // ===================================================================
    // 5. Serialize to binary
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
    // meshlet array
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outMeshlets.data()),
        reinterpret_cast<const uint8_t*>(outMeshlets.data()) + outMeshlets.size() * sizeof(meshopt_Meshlet));
    // meshlet vertices count
    AppendToBytes(bytes, static_cast<uint32_t>(meshletVertices.size()));
    // meshlet vertices array
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(meshletVertices.data()),
        reinterpret_cast<const uint8_t*>(meshletVertices.data()) + meshletVertices.size() * sizeof(uint32_t));
    // meshlet triangles count (in uint32_t units)
    AppendToBytes(bytes, static_cast<uint32_t>(outTriangles.size()));
    // meshlet triangles array (uint32_t[])
    bytes.insert(bytes.end(),
        reinterpret_cast<const uint8_t*>(outTriangles.data()),
        reinterpret_cast<const uint8_t*>(outTriangles.data()) + outTriangles.size() * sizeof(uint32_t));

    // ===================================================================
    // 6. Write to file using engine's FileUtil
    // ===================================================================
    if (!baselib::FileUtil::WriteBinaryFile(outputFile, bytes))
        return 1;

    std::cout << "Successfully wrote " << bytes.size() << " bytes to " << outputFile << std::endl;

    std::cout << "\nMeshlet details:" << std::endl;
    for (size_t i = 0; i < meshletCount; ++i)
    {
        const auto& m = outMeshlets[i];
        std::cout << "  Meshlet[" << i << "]: vertices=" << m.vertex_count
                  << ", triangles=" << m.triangle_count
                  << ", vertex_offset=" << m.vertex_offset
                  << ", triangle_offset=" << m.triangle_offset
                  << std::endl;
    }

    return 0;
}
