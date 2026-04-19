//
//  GeoMipTerrain.h
//  GNXEngine
//
//  GeoMipMapping terrain with LOD support and crack fixing.
//  Generates a Mesh that can be used with MeshRenderer,
//  and provides per-patch LOD selection via UpdateLOD().
//

#ifndef GNXENGINE_GEOMIP_TERRAIN_INCLUDE_H
#define GNXENGINE_GEOMIP_TERRAIN_INCLUDE_H

#include "../RSDefine.h"
#include "../mesh/Mesh.h"
#include "Runtime/MathUtil/include/AABB.h"

NS_RENDERSYSTEM_BEGIN

// Patch metadata (public for frustum culling access)
struct PatchInfo
{
    uint32_t startX = 0;
    uint32_t startZ = 0;
    float centerX = 0.0f;
    float centerZ = 0.0f;
    mathutil::AxisAlignedBoxf worldBounds;  // world-space AABB for frustum culling
};

class RENDERSYSTEM_API GeoMipTerrain
{
public:
    ~GeoMipTerrain();

    // Create from heightmap image (GRAY8 or GRAY16)
    static std::shared_ptr<GeoMipTerrain> CreateFromHeightMap(
        const char* heightmapPath,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t patchSize = 33);

    // Create from procedural sine-wave noise
    static std::shared_ptr<GeoMipTerrain> Create(
        uint32_t gridSize = 513,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t patchSize = 33);

    // Rebuild index buffer based on camera position (call once per frame)
    void UpdateLOD(const mathutil::Vector3f& cameraPos);

    // Get the mesh (for use with MeshRenderer / scene system)
    MeshPtr GetMesh() const { return mMesh; }

    // Get height at world position (bilinear interpolation)
    float GetHeight(float worldX, float worldZ) const;

    // Configure LOD distance thresholds.
    // distances[i] = max distance for LOD level i (should be increasing).
    void SetLODDistances(const std::vector<float>& distances);

    uint32_t GetMaxLOD() const { return mMaxLOD; }
    uint32_t GetPatchCount() const { return (uint32_t)mPatches.size(); }
    uint32_t GetPatchesPerSide() const { return mPatchesPerSide; }
    const std::vector<PatchInfo>& GetPatches() const { return mPatches; }
    float GetWorldSize() const { return mWorldSize; }
    float GetHeightScale() const { return mHeightScale; }
    uint32_t GetGridSize() const { return mGridSize; }

private:
    GeoMipTerrain();

    void InitVertexData(
        const std::vector<mathutil::Vector3f>& positions,
        const std::vector<mathutil::Vector3f>& normals,
        const std::vector<mathutil::Vector4f>& tangents,
        const std::vector<mathutil::Vector2f>& uvs);

    void GenerateLODIndexTemplates();

    // Two-pass LOD update (Terrain7 algorithm)
    void UpdateLODMapPass1(const mathutil::Vector3f& cameraPos);
    void UpdateLODMapPass2();
    uint32_t DistanceToLOD(float distance) const;

    // Per-patch LOD selection (simple, for fallback)
    uint32_t SelectLOD(const mathutil::Vector3f& cameraPos, uint32_t patchIdx) const;

    // Procedural height function (same as TerrainGenerator)
    static float ComputeHeight(float x, float z);

    // Integer power helper
    static uint32_t PowInt(uint32_t base, uint32_t exp);

    // Crack-fixing triangle fan for a single LOD cell
    // stride = global grid width (mGridSize), used as row stride in index computation.
    // Indices are patch-local in position but use global row stride so that
    // baseVertex (patchStartZ * gridSize + patchStartX) correctly offsets them.
    static void CreateTriangleFan(
        std::vector<uint32_t>& indices,
        uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight,
        uint32_t lodTop, uint32_t lodBottom,
        uint32_t x, uint32_t z, uint32_t stride);

    // Height data (world-space Y, i.e. already scaled)
    std::vector<float> mHeightMap;
    uint32_t mGridSize = 0;
    uint32_t mPatchSize = 33;
    uint32_t mPatchesPerSide = 0;
    float mWorldSize = 512.0f;
    float mHeightScale = 80.0f;

    // GPU resources
    MeshPtr mMesh;

    // Per-LOD, per-neighbor-permutation index info
    // The master index buffer is built once at init time and NEVER changes.
    // Each permutation stores its {start, count} range within the master buffer.
    // At render time, per-patch SubMeshInfo entries reference these ranges
    // and use baseVertex to offset into the vertex buffer.
    struct SinglePermutationInfo
    {
        uint32_t start = 0;  // first index in the master index buffer
        uint32_t count = 0;  // number of indices
    };

    struct LODPermutationInfo
    {
        SinglePermutationInfo info[2][2][2][2]; // [left][right][top][bottom]
    };
    std::vector<LODPermutationInfo> mLODPermutationInfo; // indexed by [lod]

    // Patch metadata
    std::vector<PatchInfo> mPatches;

    // Per-patch LOD info (updated each frame by two-pass algorithm)
    struct PatchLod
    {
        uint32_t core = 0;
        uint32_t left = 0;   // 0 = neighbor same/lower LOD, 1 = neighbor one LOD coarser
        uint32_t right = 0;
        uint32_t top = 0;
        uint32_t bottom = 0;
    };
    std::vector<PatchLod> mPatchLods;

    // LOD configuration
    std::vector<float> mLODDistances;
    uint32_t mMaxLOD = 0;
};

typedef std::shared_ptr<GeoMipTerrain> GeoMipTerrainPtr;

NS_RENDERSYSTEM_END

#endif
