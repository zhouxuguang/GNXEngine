//
//  QuadTreeTerrain.h
//  GNXEngine
//
//  Quadtree-based terrain with adaptive LOD.
//  Subdivides the terrain based on camera distance: close regions are
//  tessellated into small high-detail patches, distant regions use
//  large coarse patches. Neighbor constraint ensures adjacent leaves
//  differ by at most 1 level (required for crack fixing).
//

#ifndef GNXENGINE_QUADTREE_TERRAIN_INCLUDE_H
#define GNXENGINE_QUADTREE_TERRAIN_INCLUDE_H

#include "../RSDefine.h"
#include "../mesh/Mesh.h"
#include "Runtime/MathUtil/include/AABB.h"
#include "Runtime/MathUtil/include/Frustum.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API QuadTreeTerrain
{
public:
    ~QuadTreeTerrain();

    // Create from heightmap image (GRAY8 or GRAY16)
    static std::shared_ptr<QuadTreeTerrain> CreateFromHeightMap(
        const char* heightmapPath,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t maxLevel = 5);

    // Create from procedural sine-wave noise
    static std::shared_ptr<QuadTreeTerrain> Create(
        uint32_t gridSize = 513,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t maxLevel = 5);

    // Update quadtree LOD based on camera (call once per frame)
    void Update(const mathutil::Vector3f& cameraPos,
                float fovY = 0.0f,
                float screenHeight = 0.0f);

    // Get the mesh (for use with rendering pipeline)
    MeshPtr GetMesh() const { return mMesh; }

    // Get height at world position (bilinear interpolation)
    float GetHeight(float worldX, float worldZ) const;

    // Configuration
    void SetLODDistanceFactor(float factor);
    void SetSSEThreshold(float threshold);

    uint32_t GetMaxLevel() const { return mMaxLevel; }
    uint32_t GetLeafCount() const { return (uint32_t)mLeafNodes.size(); }
    const std::vector<mathutil::AxisAlignedBoxf>& GetLeafBounds() const { return mLeafBounds; }
    float GetWorldSize() const { return mWorldSize; }
    float GetHeightScale() const { return mHeightScale; }
    uint32_t GetGridSize() const { return mGridSize; }

    // Per-patch instance data for GPU instanced rendering.
    // One entry per visible (non-culled) leaf node per frame.
    // Layout must match HLSL PatchInstanceData in TerrainBasePassInstanced.shader
    // and TerrainDepthInstanced.shader exactly (same fields, same order).
    struct PatchInstanceData
    {
        uint32_t baseVertex;   // VB offset: leaf->z * mGridSize + leaf->x
        uint32_t indexStart;   // offset into mMasterIndices for this patch's indices
        // NOTE: indexCount is NOT here — the shader uses the uniform dummy IB size
        //       (mUniformPatchIndexCount) as the per-instance vertex count via SV_VertexID.
        //       Keeping this struct identical between C++ and HLSL is critical.
        uint32_t _pad0 = 0;   // explicit padding to 16 bytes for StructuredBuffer alignment
        uint32_t _pad1 = 0;   // (HLSL StructuredBuffer elements are 16-byte aligned)
    };

    // Get patch instance data for GPU instanced rendering.
    // Fills mPatchInstances with data for leaves that pass frustum culling.
    // Also computes mMaxPatchIndexCount (for dummy IB sizing).
    // Call after Update() each frame.
    void BuildPatchInstances(const mathutil::Frustumf* frustum = nullptr);

    // Accessors for instanced rendering
    const std::vector<PatchInstanceData>& GetPatchInstances() const { return mPatchInstances; }
    uint32_t GetPatchInstanceCount() const { return (uint32_t)mPatchInstances.size(); }
    uint32_t GetMaxPatchIndexCount() const { return mMaxPatchIndexCount; }

    // Uniform index count: all static pool entries are padded to this size.
    // Required by DrawIndexedInstancePrimitives which uses one index count for all instances.
    uint32_t GetUniformPatchIndexCount() const { return mUniformPatchIndexCount; }

private:
    QuadTreeTerrain();

    // Quadtree node
    struct Node
    {
        uint32_t x = 0;           // top-left corner in grid coordinates
        uint32_t z = 0;
        uint32_t size = 0;        // size in grid cells (power of 2)
        uint32_t level = 0;       // depth in tree (0 = root)
        mathutil::AxisAlignedBoxf bounds;
        Node* parent = nullptr;   // parent pointer for neighbor traversal

        // Children: [0]=NW, [1]=NE, [2]=SW, [3]=SE
        std::unique_ptr<Node> children[4];

        bool IsLeaf() const { return children[0] == nullptr; }
    };

    // Vertex data initialization
    void InitVertexData(
        const std::vector<mathutil::Vector3f>& positions,
        const std::vector<mathutil::Vector3f>& normals,
        const std::vector<mathutil::Vector4f>& tangents,
        const std::vector<mathutil::Vector2f>& uvs);

    // Quadtree operations
    void BuildNode(Node* node);
    void UpdateNode(Node* node, const mathutil::Vector3f& cameraPos);
    bool ShouldSubdivide(const Node& node, const mathutil::Vector3f& cameraPos) const;
    void Subdivide(Node* node);
    void CollectLeaves(Node* node);
    void ComputeNodeBounds(Node* node);

    // Neighbor constraint: ensure adjacent leaves differ by at most 1 level
    static int GetChildIndex(const Node* node);
    uint32_t GetMaxNeighborLevel(const Node* node, int direction) const;
    void EnforceNeighborConstraint();

    // Per-leaf neighbor LOD info for crack-fixing triangle fan
    struct LeafNeighborInfo
    {
        uint8_t leftCoarser   = 0;  // 1 if left  neighbor (-X) is coarser (lower level)
        uint8_t rightCoarser  = 0;  // 1 if right neighbor (+X) is coarser
        uint8_t topCoarser    = 0;  // 1 if top   neighbor (-Z) is coarser
        uint8_t bottomCoarser = 0;  // 1 if bottom neighbor (+Z) is coarser
    };

    // Static index pool: pre-computed index buffers for all (stride, permutation) combos.
    // Built once at init time, never changed after. Eliminates per-frame IB upload.
    struct IndexPoolEntry
    {
        uint32_t start = 0;   // offset into mMasterIndices
        uint32_t count = 0;   // number of indices for this entry
    };

    // Build the static index pool (call once after InitVertexData)
    void BuildStaticIndexPool();

    // Build SubMeshInfo list from current leaf nodes (lookup into static pool)
    void GenerateLeafMesh();

    // Generate triangle-fan indices for a single fan-cell (SEMI-LOCAL coordinates).
    // Uses mGridSize as row stride so baseVertex correctly offsets to global VB.
    void CreateTriangleFanLocal(std::vector<uint32_t>& indices,
                                uint32_t fcx, uint32_t fcz,
                                uint32_t stride, uint32_t globalGridSize,
                                bool leftCoarser, bool rightCoarser,
                                bool topCoarser, bool bottomCoarser);

    // Map stride value → stride level index (log2)
    uint32_t GetStrideLevel(uint32_t stride) const;

    // Procedural height function
    static float ComputeHeight(float x, float z);

    // Height data
    std::vector<float> mHeightMap;
    uint32_t mGridSize = 0;          // vertex count per side (2^n + 1)
    uint32_t mGridSizeCells = 0;     // cell count per side (2^n)
    float mWorldSize = 512.0f;
    float mHeightScale = 80.0f;
    uint32_t mMaxLevel = 5;          // maximum quadtree depth

    // Quadtree root
    Node mRoot;
    std::vector<Node*> mLeafNodes;   // collected each frame for rendering
    std::vector<mathutil::AxisAlignedBoxf> mLeafBounds; // AABB per leaf, for frustum culling
    std::vector<LeafNeighborInfo> mLeafNeighborInfo;     // neighbor LOD info per leaf

    // Static index pool (built once at init, read-only at runtime)
    std::vector<std::vector<IndexPoolEntry>> mIndexPool;  // [strideLevel][16 permutations]
    std::vector<uint32_t> mMasterIndices;                  // contiguous master index buffer

    // Per-patch instance data for instanced rendering (rebuilt each frame)
    std::vector<PatchInstanceData> mPatchInstances;
    uint32_t mMaxPatchIndexCount = 0;   // max index count across all patches this frame
    uint32_t mUniformPatchIndexCount = 0; // uniform size all pool entries are padded to

    // GPU resources
    MeshPtr mMesh;

    // LOD configuration
    float mLODDistanceFactor = 1.0f; // distance = nodeWorldSize * factor
    float mSSEThreshold = 4.0f;
    float mTanHalfFovY = 0.0f;
    float mScreenHeight = 0.0f;

    // Each leaf renders as a grid with this many vertices per side
    static constexpr uint32_t kLeafVerticesPerSide = 17;
};

typedef std::shared_ptr<QuadTreeTerrain> QuadTreeTerrainPtr;

NS_RENDERSYSTEM_END

#endif
