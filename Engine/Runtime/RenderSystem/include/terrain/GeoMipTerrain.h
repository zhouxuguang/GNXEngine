//
//  GeoMipTerrain.h
//  GNXEngine
//
//  GeoMipMapping terrain with LOD support.
//  Generates a Mesh that can be used with MeshRenderer,
//  and provides per-patch LOD selection via UpdateLOD().
//

#ifndef GNXENGINE_GEOMIP_TERRAIN_INCLUDE_H
#define GNXENGINE_GEOMIP_TERRAIN_INCLUDE_H

#include "../RSDefine.h"
#include "../mesh/Mesh.h"

NS_RENDERSYSTEM_BEGIN

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
    uint32_t SelectLOD(const mathutil::Vector3f& cameraPos, uint32_t patchIdx) const;

    // Procedural height function (same as TerrainGenerator)
    static float ComputeHeight(float x, float z);

    // Height data (world-space Y, i.e. already scaled)
    std::vector<float> mHeightMap;
    uint32_t mGridSize = 0;
    uint32_t mPatchSize = 33;
    uint32_t mPatchesPerSide = 0;
    float mWorldSize = 512.0f;
    float mHeightScale = 80.0f;

    // GPU resources
    MeshPtr mMesh;

    // Per-LOD index templates (local indices within a patch, 0-based)
    // mLODIndexTemplates[lod] = flat array of local vertex indices
    std::vector<std::vector<uint32_t>> mLODIndexTemplates;

    // Per-LOD index count (number of indices per patch at this LOD)
    std::vector<uint32_t> mLODIndexCount;

    // Patch metadata
    struct PatchInfo
    {
        uint32_t startX = 0;
        uint32_t startZ = 0;
        float centerX = 0.0f;
        float centerZ = 0.0f;
    };
    std::vector<PatchInfo> mPatches;

    // LOD configuration
    std::vector<float> mLODDistances;
    uint32_t mMaxLOD = 0;
};

typedef std::shared_ptr<GeoMipTerrain> GeoMipTerrainPtr;

NS_RENDERSYSTEM_END

#endif
