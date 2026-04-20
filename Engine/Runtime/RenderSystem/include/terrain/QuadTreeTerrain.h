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

    // Build index buffer and SubMeshInfo list from current leaf nodes
    void GenerateLeafMesh();

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
