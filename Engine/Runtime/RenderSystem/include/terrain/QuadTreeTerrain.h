//
//  QuadTreeTerrain.h
//  GNXEngine
//
//  基于四叉树的自适应 LOD 地形系统。
//  根据相机距离细分地形：近处区域被细分为小尺寸高细节 patch，
//  远处区域使用大尺寸粗粒度 patch。邻居约束确保相邻叶节点
// 层级差不超过 1（裂缝修复的必要条件）。
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

// GPU 驱动地形渲染的叶节点元数据（SSBO）。
// 32 字节，按 vec4 对齐以便 GPU 访问。
struct PatchMeta
{
    float worldX;       // patch 左上角的世界 X 坐标
    float worldZ;       // patch 左上角的世界 Z 坐标
    float worldSize;    // patch 的世界空间尺寸
    float minHeight;    // AABB 最小 Y（用于遮挡剔除）

    uint32_t gridX;     // 网格 X 起始坐标
    uint32_t gridZ;     // 网格 Z 起始坐标
    uint32_t gridSize;  // 网格单元尺寸
    uint32_t level;     // 四叉树深度（LOD 层级）
};

class RENDERSYSTEM_API QuadTreeTerrain
{
public:
    ~QuadTreeTerrain();

    // 从高度图图像创建（GRAY8 或 GRAY16）
    static std::shared_ptr<QuadTreeTerrain> CreateFromHeightMap(
        const char* heightmapPath,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t maxLevel = 5);

    // 从程序化正弦波噪声创建
    static std::shared_ptr<QuadTreeTerrain> Create(
        uint32_t gridSize = 513,
        float worldSizeXZ = 512.0f,
        float heightScale = 80.0f,
        uint32_t maxLevel = 5);

    // 根据相机位置更新四叉树 LOD（每帧调用一次）
    void Update(const mathutil::Vector3f& cameraPos,
                float fovY = 0.0f,
                float screenHeight = 0.0f);

    // 获取网格（供渲染管线使用）
    MeshPtr GetMesh() const { return mMesh; }

    // 获取世界位置的高度（双线性插值）
    float GetHeight(float worldX, float worldZ) const;

    // 配置参数
    void SetLODDistanceFactor(float factor);
    void SetSSEThreshold(float threshold);

    // 从可见叶节点构建间接绘制命令（可选视锥体剔除）
    void BuildIndirectCommands(const mathutil::Frustumf* frustum = nullptr);
    const std::vector<RenderCore::DrawIndexedIndirectCommand>& GetIndirectCommands() const { return mIndirectCommands; }
    uint32_t GetIndirectDrawCount() const { return (uint32_t)mIndirectCommands.size(); }

    // 构建 GPU 路径数据：可见 PatchMeta SSBO + 模板网格间接绘制命令
    void BuildGPUPathData(const mathutil::Frustumf* frustum = nullptr);

    uint32_t GetMaxLevel() const { return mMaxLevel; }
    uint32_t GetLeafCount() const { return (uint32_t)mLeafNodes.size(); }
    const std::vector<mathutil::AxisAlignedBoxf>& GetLeafBounds() const { return mLeafBounds; }
    float GetWorldSize() const { return mWorldSize; }
    float GetHeightScale() const { return mHeightScale; }
    uint32_t GetGridSize() const { return mGridSize; }

    // GPU 驱动渲染的 GPU 资源
    RenderCore::RCTexture2DPtr GetHeightMapTexture() const { return mHeightMapTexture; }
    RenderCore::RCBufferPtr GetPatchMetaBuffer() const { return mPatchMetaBuffer; }
    uint32_t GetPatchMetaCount() const { return (uint32_t)mPatchMetaData.size(); }
    RenderCore::RCBufferPtr GetVisiblePatchMetaBuffer() const { return mVisiblePatchMetaBuffer; }
    uint32_t GetVisiblePatchMetaCount() const { return (uint32_t)mVisiblePatchMeta.size(); }

    // GPU 驱动渲染的模板网格
    RenderCore::RCBufferPtr GetTemplateVB() const { return mTemplateVB; }
    RenderCore::IndexBufferPtr GetTemplateIB() const { return mTemplateIB; }
    uint32_t GetTemplatePositionSize() const { return mTemplatePositionSize; }

private:
    QuadTreeTerrain();

    // 四叉树节点
    struct Node
    {
        uint32_t x = 0;           // 网格坐标中的左上角位置
        uint32_t z = 0;
        uint32_t size = 0;        // 网格单元尺寸（2 的幂）
        uint32_t level = 0;       // LOD 层级：mMaxLevel=最粗（根节点），0=最细
        float maxGeoError = 0.0f; // 以该 LOD 渲染时的最大几何误差
        mathutil::AxisAlignedBoxf bounds;
        Node* parent = nullptr;   // 父节点指针，用于邻居遍历

        // 子节点：[0]=西北, [1]=东北, [2]=西南, [3]=东南
        std::unique_ptr<Node> children[4];

        bool IsLeaf() const { return children[0] == nullptr; }
    };

    // 顶点数据初始化
    void InitVertexData(
        const std::vector<mathutil::Vector3f>& positions,
        const std::vector<mathutil::Vector3f>& normals,
        const std::vector<mathutil::Vector4f>& tangents,
        const std::vector<mathutil::Vector2f>& uvs);

    // 将高度图上传为 GPU 纹理（初始化时调用一次）
    void CreateHeightMapTexture();

    // 构建并上传 PatchMeta SSBO（每帧在 Update 中调用）
    void BuildPatchMetaBuffer();

    // 创建模板网格（17x17），用于 GPU 驱动渲染
    void CreateTemplateMesh();

    // 四叉树操作
    void BuildNode(Node* node);
    void UpdateNode(Node* node, const mathutil::Vector3f& cameraPos);
    bool ShouldSubdivide(const Node& node, const mathutil::Vector3f& cameraPos) const;
    void Subdivide(Node* node);
    void CollectLeaves(Node* node);
    void ComputeNodeBounds(Node* node);

    // 几何误差计算
    void ComputeAllGeoErrors();
    float ComputeNodeGeoError(uint32_t x, uint32_t z, uint32_t size) const;
    float GetCachedGeoError(uint32_t level, uint32_t x, uint32_t z) const;

    // 邻居约束：确保相邻叶节点层级差不超过 1
    static int GetChildIndex(const Node* node);
    uint32_t GetMinNeighborLevel(const Node* node, int direction) const;
    void EnforceNeighborConstraint();

    // 每个叶节点的邻居 LOD 信息（用于裂缝修复三角形扇形）
    struct LeafNeighborInfo
    {
        uint8_t leftCoarser   = 0;  // 1 表示左邻居(-X)更粗（层级号更大）
        uint8_t rightCoarser  = 0;  // 1 表示右邻居(+X)更粗
        uint8_t topCoarser    = 0;  // 1 表示上邻居(-Z)更粗
        uint8_t bottomCoarser = 0;  // 1 表示下邻居(+Z)更粗
    };

    // 静态索引池：预计算所有(步长, 排列)组合的索引缓冲区。
    // 初始化时构建一次，之后不再修改。消除每帧 IB 上传。
    struct IndexPoolEntry
    {
        uint32_t start = 0;   // mMasterIndices 中的偏移量
        uint32_t count = 0;   // 该条目的索引数量
    };

    // 构建静态索引池（在 InitVertexData 之后调用一次）
    void BuildStaticIndexPool();

    // 从当前叶节点构建 SubMeshInfo 列表（查找静态池）
    void GenerateLeafMesh();

    // 为单个扇形单元生成三角形扇形索引（半局部坐标）。
    // 使用 mGridSize 作为行跨度，使 baseVertex 正确偏移到全局 VB。
    void CreateTriangleFanLocal(std::vector<uint32_t>& indices,
                                uint32_t fcx, uint32_t fcz,
                                uint32_t stride, uint32_t globalGridSize,
                                bool leftCoarser, bool rightCoarser,
                                bool topCoarser, bool bottomCoarser);

    // 将步长值映射为步长层级索引（log2）
    uint32_t GetStrideLevel(uint32_t stride) const;

    // 程序化高度函数
    static float ComputeHeight(float x, float z);

    // 高度数据
    std::vector<float> mHeightMap;
    uint32_t mGridSize = 0;          // 每边顶点数（2^n + 1）
    uint32_t mGridSizeCells = 0;     // 每边单元数（2^n）
    float mWorldSize = 512.0f;
    float mHeightScale = 80.0f;
    uint32_t mMaxLevel = 5;          // 最大四叉树深度

    // 四叉树根节点
    Node mRoot;
    std::vector<Node*> mLeafNodes;   // 每帧收集的叶节点，用于渲染
    std::vector<mathutil::AxisAlignedBoxf> mLeafBounds; // 每个叶节点的 AABB，用于视锥体剔除
    std::vector<LeafNeighborInfo> mLeafNeighborInfo;     // 每个叶节点的邻居 LOD 信息

    // 静态索引池（初始化时构建一次，运行时只读）
    std::vector<std::vector<IndexPoolEntry>> mIndexPool;  // [步长层级][16 种排列]
    std::vector<uint32_t> mMasterIndices;                  // 连续的主索引缓冲区

    // 预计算的几何误差缓存：mGeoErrorCache[level][j * nodesPerSide + i]
    std::vector<std::vector<float>> mGeoErrorCache;

    // GPU 资源
    MeshPtr mMesh;

    // GPU 驱动渲染的 GPU 资源（阶段1A+1B）
    RenderCore::RCTexture2DPtr mHeightMapTexture;   // 高度图 R32Float 纹理（创建一次）
    RenderCore::RCBufferPtr mPatchMetaBuffer;        // PatchMeta[] SSBO（每帧重建）
    std::vector<PatchMeta> mPatchMetaData;           // CPU 端 PatchMeta 数据
    std::vector<PatchMeta> mVisiblePatchMeta;        // 可见 PatchMeta（视锥体剔除后）
    RenderCore::RCBufferPtr mVisiblePatchMetaBuffer; // 仅可见 patch 的 SSBO

    // GPU 驱动渲染的模板网格（17x17 = 289 顶点, 1536 索引）
    RenderCore::RCBufferPtr  mTemplateVB;            // 顶点缓冲区（SoA: 位置 + 纹理坐标）
    RenderCore::IndexBufferPtr mTemplateIB;          // 索引缓冲区
    uint32_t mTemplatePositionSize = 0;              // 模板 VB 中纹理坐标数据的字节偏移

    // 间接绘制命令（每帧由 BuildIndirectCommands 构建）
    std::vector<RenderCore::DrawIndexedIndirectCommand> mIndirectCommands;

    // LOD 配置
    float mLODDistanceFactor = 2.0f; // 阈值 = 节点世界尺寸 * factor（中心点距离需要比最近点距离更大的因子）
    float mSSEThreshold = 32.0f;
    float mTanHalfFovY = 0.0f;
    float mScreenHeight = 0.0f;

    // 每个叶节点渲染为每边这么多顶点的网格（9x9 = 81 顶点，8x8 = 64 三角形）
    static constexpr uint32_t kLeafVerticesPerSide = 9;
    
    // LOD 稳定性管理 - 防止 LOD 快速切换
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
