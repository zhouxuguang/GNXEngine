//
//  MeshLetBuilder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2024/6/14.
//

#ifndef GNXENGINE_MESHLET_BUILDER_INCLUDE_H
#define GNXENGINE_MESHLET_BUILDER_INCLUDE_H

#include "../RSDefine.h"
#include "MeshLetCommon.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

// metis idx_t = int64_t (METIS_BUILD_64BIT=1)
using metis_idx_t = int64_t;

NS_RENDERSYSTEM_BEGIN

// -----------------------------------------------------------------------
// MeshletBuilder
// -----------------------------------------------------------------------
// 负责从原始网格数据构建 meshlet 的构建器。
// 支持：
//   - 顶点位置去重
//   - 多级 LOD 简化（meshopt_simplify）
//   - meshlet 构建（meshopt_buildMeshlets）
//   - metis 图分区优化
//   - 包围球计算
class MeshletBuilder
{
public:
    MeshletBuilder();
    ~MeshletBuilder();

    // 设置 meshlet 最大顶点数（默认 64）
    void SetMaxVertices(uint32_t maxVerts) { mMaxVertices = maxVerts; }
    uint32_t GetMaxVertices() const { return mMaxVertices; }

    // 设置 meshlet 最大三角形数（默认 124，需 4 对齐）
    void SetMaxTriangles(uint32_t maxTris) { mMaxTriangles = maxTris; }
    uint32_t GetMaxTriangles() const { return mMaxTriangles; }

    // 设置简化目标误差（默认 0.01）
    void SetTargetError(float error) { mTargetError = error; }
    float GetTargetError() const { return mTargetError; }

    // 设置最大 LOD 级别数（默认 5）
    void SetMaxLodLevels(int levels) { mMaxLodLevels = levels; }
    int GetMaxLodLevels() const { return mMaxLodLevels; }

    // 设置最小三角形数（低于此值不再简化，默认 256）
    void SetMinTriangles(size_t minTris) { mMinTriangles = minTris; }
    size_t GetMinTriangles() const { return mMinTriangles; }

    // 设置 LOD 简化比例（默认 0.5，即每级减半）
    void SetLodSimplifyRatio(float ratio) { mLodSimplifyRatio = ratio; }
    float GetLodSimplifyRatio() const { return mLodSimplifyRatio; }

    // -------- 构建 --------

    // 从原始顶点位置和索引构建 meshlet（含多 LOD）
    // @param positions     顶点位置数组 (float[vertexCount * 3])
    // @param vertexCount   顶点个数
    // @param indices       三角形索引数组
    // @param indexCount    索引个数
    // @param outData       输出的 meshlet 文件数据
    // @return 是否构建成功
    bool Build(
        const float*    positions,
        size_t          vertexCount,
        const uint32_t* indices,
        size_t          indexCount,
        struct MeshletFileData& outData);

private:

    // 单 LOD meshlet 构建
    void BuildSingleLod(
        const uint32_t* indices,
        size_t          indexCount,
        const float*    positions,
        size_t          vertexCount,
        std::vector<Meshlet>&    outMeshlets,
        std::vector<uint32_t>&   outVertices,
        std::vector<uint32_t>&   outTriangles);

    /**
     * 构建 meshlet 之间的连通图（CSR 格式，适用于 METIS）
     *
     * 算法：
     *   1. 遍历每个 meshlet 的所有三角形，提取无序边 (min(vi,vj), max(vi,vj))
     *   2. 建立 edge -> set<meshletId> 映射（只记录包含该边的 meshlet）
     *   3. 出现在 ≥2 个 meshlet 中的边 = 共享边界
     *   4. 聚合：对 meshlet (a,b)，设 weight[a][b] = 两 meshlet 共享边的总数
     *   5. 输出 METIS CSR 格式的邻接表
     *
     * @param meshlets           meshlet 数组
     * @param meshletVertices    meshlet 顶点索引数组 (local -> global)
     * @param meshletTriangles   meshlet 打包三角形索引 (uint32_t, 每3byte一个local索引)
     * @param globalIndices      全局三角形索引（用于解析 meshlet 每个三角形的全局顶点）
     * @param indexCount         全局索引个数
     * @param outXadj            输出 CSR xadj (size = meshletCount + 1)
     * @param outAdjncy          输出 CSR adjncy (size = 2*边数)
     * @param outAdjwgt          输出 CSR adjwgt (size = 2*边数, 即共享边数量)
     */
    void BuildConnectivityGraph(
        const std::vector<Meshlet>&    meshlets,
        const std::vector<uint32_t>&   meshletVertices,
        const std::vector<uint32_t>&   meshletTriangles,
        const uint32_t*                globalIndices,
        size_t                         indexCount,
        std::vector<metis_idx_t>&      outXadj,
        std::vector<metis_idx_t>&      outAdjncy,
        std::vector<metis_idx_t>&      outAdjwgt);

    /**
     * 使用 METIS_PartGraphKway 对 meshlet 进行 k-way 分区
     *
     * 目标：最大化簇组内部的共享边界（连通性），最小化簇组之间的共享边界。
     *
     * @param meshletCount   meshlet 数量（图的顶点数）
     * @param xadj           CSR 邻接表偏移
     * @param adjncy         CSR 邻接表
     * @param adjwgt         CSR 边权重
     * @param numParts       目标分区数 k
     * @param outPart        输出分区结果 (size = meshletCount, part[i] ∈ [0, k-1])
     * @return 是否分区成功
     */
    bool PartitionWithMetis(
        metis_idx_t                meshletCount,
        const std::vector<metis_idx_t>& xadj,
        const std::vector<metis_idx_t>& adjncy,
        const std::vector<metis_idx_t>& adjwgt,
        metis_idx_t                numParts,
        std::vector<metis_idx_t>&  outPart);

    // 配置参数
    uint32_t mMaxVertices      = kMeshletMaxVertices;
    uint32_t mMaxTriangles     = kMeshletMaxTriangles;
    float    mTargetError      = 0.01f;
    int      mMaxLodLevels     = 20;
    size_t   mMinTriangles     = kMeshletMaxTriangles;
    float    mLodSimplifyRatio = 0.5f;
};

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MESHLET_BUILDER_INCLUDE_H
