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
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"

NS_RENDERSYSTEM_BEGIN

// Per-leaf metadata for GPU-driven terrain rendering (SSBO).
// 32 bytes, aligned to vec4 for GPU access.
struct PatchMeta
{
    float worldX;       // world X of patch top-left corner
    float worldZ;       // world Z of patch top-left corner
    float worldSize;    // world-space size of the patch
    float minHeight;    // AABB min Y (for occlusion culling)

    uint32_t gridX;     // grid X start coordinate
    uint32_t gridZ;     // grid Z start coordinate
    uint32_t gridSize;  // size in grid cells
    uint32_t level;     // quadtree depth (LOD level)
};

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

    // Build indirect draw commands from visible leaves (with optional frustum culling)
    void BuildIndirectCommands(const mathutil::Frustumf* frustum = nullptr);
    const std::vector<RenderCore::DrawIndexedIndirectCommand>& GetIndirectCommands() const { return mIndirectCommands; }
    uint32_t GetIndirectDrawCount() const { return (uint32_t)mIndirectCommands.size(); }

    // Build GPU path data: visible PatchMeta SSBO + template mesh indirect commands
    void BuildGPUPathData(const mathutil::Frustumf* frustum = nullptr);

    uint32_t GetMaxLevel() const { return mMaxLevel; }
    uint32_t GetLeafCount() const { return (uint32_t)mLeafNodes.size(); }
    const std::vector<mathutil::AxisAlignedBoxf>& GetLeafBounds() const { return mLeafBounds; }
    float GetWorldSize() const { return mWorldSize; }
    float GetHeightScale() const { return mHeightScale; }
    uint32_t GetGridSize() const { return mGridSize; }

    // GPU resources for GPU-driven rendering
    RenderCore::RCTexture2DPtr GetHeightMapTexture() const { return mHeightMapTexture; }
    RenderCore::RCBufferPtr GetPatchMetaBuffer() const { return mPatchMetaBuffer; }
    uint32_t GetPatchMetaCount() const { return (uint32_t)mPatchMetaData.size(); }
    RenderCore::RCBufferPtr GetVisiblePatchMetaBuffer() const { return mVisiblePatchMetaBuffer; }
    uint32_t GetVisiblePatchMetaCount() const { return (uint32_t)mVisiblePatchMeta.size(); }

    // Template mesh for GPU-driven rendering
    RenderCore::RCBufferPtr GetTemplateVB() const { return mTemplateVB; }
    RenderCore::IndexBufferPtr GetTemplateIB() const { return mTemplateIB; }
    uint32_t GetTemplatePositionSize() const { return mTemplatePositionSize; }

private:
    QuadTreeTerrain();

    // Quadtree node
    struct Node
    {
        uint32_t x = 0;           // top-left corner in grid coordinates
        uint32_t z = 0;
        uint32_t size = 0;        // size in grid cells (power of 2)
        uint32_t level = 0;       // LOD level: mMaxLevel=coarsest (root), 0=finest
        float maxGeoError = 0.0f; // max geometric error when rendered at this LOD
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

    // Upload heightmap as GPU texture (called once during init)
    void CreateHeightMapTexture();

    // Build and upload PatchMeta SSBO (called each frame in Update)
    void BuildPatchMetaBuffer();

    // Create template mesh (17x17) for GPU-driven rendering
    void CreateTemplateMesh();

    // Quadtree operations
    void BuildNode(Node* node);
    void UpdateNode(Node* node, const mathutil::Vector3f& cameraPos);
    bool ShouldSubdivide(const Node& node, const mathutil::Vector3f& cameraPos) const;
    void Subdivide(Node* node);
    void CollectLeaves(Node* node);
    void ComputeNodeBounds(Node* node);

    // Geometric error computation
    void ComputeAllGeoErrors();
    float ComputeNodeGeoError(uint32_t x, uint32_t z, uint32_t size) const;
    float GetCachedGeoError(uint32_t level, uint32_t x, uint32_t z) const;

    // Neighbor constraint: ensure adjacent leaves differ by at most 1 level
    static int GetChildIndex(const Node* node);
    uint32_t GetMinNeighborLevel(const Node* node, int direction) const;
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

    // Pre-computed geometric error cache: mGeoErrorCache[level][j * nodesPerSide + i]
    std::vector<std::vector<float>> mGeoErrorCache;

    // GPU resources
    MeshPtr mMesh;

    // GPU resources for GPU-driven rendering (阶段1A+1B)
    RenderCore::RCTexture2DPtr mHeightMapTexture;   // heightmap as R32Float texture (created once)
    RenderCore::RCBufferPtr mPatchMetaBuffer;        // PatchMeta[] SSBO (rebuilt each frame)
    std::vector<PatchMeta> mPatchMetaData;           // CPU-side PatchMeta data
    std::vector<PatchMeta> mVisiblePatchMeta;        // visible PatchMeta (after frustum culling)
    RenderCore::RCBufferPtr mVisiblePatchMetaBuffer; // SSBO for visible patches only

    // Template mesh for GPU-driven rendering (17x17 = 289 verts, 1536 indices)
    RenderCore::RCBufferPtr  mTemplateVB;            // vertex buffer (SoA: positions + texCoords)
    RenderCore::IndexBufferPtr mTemplateIB;          // index buffer
    uint32_t mTemplatePositionSize = 0;              // byte offset to texCoord data in template VB

    // Indirect draw commands (built each frame by BuildIndirectCommands)
    std::vector<RenderCore::DrawIndexedIndirectCommand> mIndirectCommands;

    // LOD configuration
    float mLODDistanceFactor = 1.0f; // distance = nodeWorldSize * factor
    float mSSEThreshold = 32.0f;
    float mTanHalfFovY = 0.0f;
    float mScreenHeight = 0.0f;

    // Each leaf renders as a grid with this many vertices per side
    static constexpr uint32_t kLeafVerticesPerSide = 17;
    
    // LOD stability management - prevent rapid LOD switching
    static constexpr uint32_t LOD_STABILITY_FRAMES = 3;  // 需要连续3帧才切换LOD
    struct LODStabilityData {
        int stabilityCounter = 0;    // 稳定性计数器
        bool lastDecision = false;   // 上次的决定
    };
    std::unordered_map<const Node*, LODStabilityData> mLODStabilityMap;
};

typedef std::shared_ptr<QuadTreeTerrain> QuadTreeTerrainPtr;

NS_RENDERSYSTEM_END

#endif
