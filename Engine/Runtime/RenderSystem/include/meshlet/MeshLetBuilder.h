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
#include <meshoptimizer.h>
#include <unordered_map>
#include <vector>

// metis idx_t = int64_t (METIS_BUILD_64BIT=1)
using metis_idx_t = int64_t;

NS_RENDERSYSTEM_BEGIN

// -----------------------------------------------------------------------
// MergedGroup 合并后的组网格数据
//
// 将同一 METIS 分区内的所有 meshlet 合并为一个连通网格。
// triangleIndices 直接存储全局 vertexPositions 的索引，不复制顶点数据。
//
// 组边界（与其他组的共享边）在内部 compact 后自然变成 mesh border，
// Simplify() 内部使用 meshopt_SimplifyLockBorder 自动锁定。
struct MergedGroup
{
    std::vector<uint32_t> triangleIndices;   // 三角形索引，直接指向全局 vertexPositions
    uint32_t groupId = 0;                    // 所属分区编号

    // -----------------------------------------------------------------------
    // Simplify：一步完成 compact → meshopt_simplify(LockBorder) → map back
    //
    // @param globalPositions  全局顶点位置池 (float[totalVertices * 3])
    // @param globalVertexCount 全局顶点总数
    // @param targetIndexCount 目标三角形索引数（简化后保留的索引数量）
    // @param targetError      目标几何误差 (默认 0.01f)
    // @param outError         输出实际误差（可传 nullptr）
    //
    // 返回值：简化后的全局三角形索引数组。组边界自动锁定。
    // -----------------------------------------------------------------------
    std::vector<uint32_t> Simplify(
        const float*        globalPositions,
        size_t              globalVertexCount,
        size_t              targetIndexCount,
        float               targetError = 0.01f,
        float*              outError = nullptr) const
    {
        if (triangleIndices.empty())
            return {};

        // ---- Step 1: compact global → local（组边界自然变成 border edge）----
        std::vector<float>    localPositions;
        std::vector<uint32_t> localIndices;
        std::vector<uint32_t> localToGlobal;

        Compact(globalPositions, localPositions, localIndices, localToGlobal);

        // ---- Step 2: meshopt_simplify (LockBorder) ----
        size_t targetCount = std::min(targetIndexCount, localIndices.size());
        targetCount = (targetCount / 3) * 3;  // align to triangle
        if (targetCount < 3) targetCount = 3;

        std::vector<uint32_t> simplifiedLocal(localIndices.size());
        float error = 0.0f;
        size_t simplifiedCount = meshopt_simplify(
            simplifiedLocal.data(),
            localIndices.data(),
            localIndices.size(),
            localPositions.data(),
            localPositions.size() / 3,
            sizeof(float) * 3,
            targetCount,
            targetError,
            meshopt_SimplifyLockBorder,
            &error);

        if (outError) *outError = error;
        simplifiedLocal.resize(simplifiedCount);

        // ---- Step 3: map back local → global ----
        std::vector<uint32_t> simplifiedGlobal;
        simplifiedGlobal.reserve(simplifiedCount);
        for (uint32_t li : simplifiedLocal)
            simplifiedGlobal.push_back(localToGlobal[li]);

        return simplifiedGlobal;
    }

private:
    // 内部 compact：去重 global 索引 → 紧凑 local 数组
    void Compact(
        const float*              globalPositions,
        std::vector<float>&       outLocalPositions,
        std::vector<uint32_t>&    outLocalIndices,
        std::vector<uint32_t>&    outLocalToGlobal) const
    {
        std::unordered_map<uint32_t, uint32_t> globalToLocal;
        outLocalPositions.clear();
        outLocalToGlobal.clear();
        outLocalIndices.resize(triangleIndices.size());

        for (size_t i = 0; i < triangleIndices.size(); ++i)
        {
            uint32_t gi = triangleIndices[i];
            auto it = globalToLocal.find(gi);
            if (it == globalToLocal.end())
            {
                uint32_t li = static_cast<uint32_t>(outLocalToGlobal.size());
                globalToLocal[gi] = li;
                outLocalToGlobal.push_back(gi);
                outLocalPositions.push_back(globalPositions[gi * 3 + 0]);
                outLocalPositions.push_back(globalPositions[gi * 3 + 1]);
                outLocalPositions.push_back(globalPositions[gi * 3 + 2]);
                outLocalIndices[i] = li;
            }
            else
            {
                outLocalIndices[i] = it->second;
            }
        }
    }
};

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
class RENDERSYSTEM_API MeshletBuilder
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

    // 设置 METIS 分区数（默认 0=自动 sqrt(|V|)，设为 2=分成两组调试）
    void SetNumPartitions(uint32_t n) { mNumPartitions = n; }
    uint32_t GetNumPartitions() const { return mNumPartitions; }

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

    /**
     * 将同组 meshlet 合并为一个完整网格
     *
     * @param inData      已分区好的 meshlet 数据（含 meshletPartitions）
     * @param outGroups   输出的各组合并网格，大小 = numPartitions
     *
     * 合并过程：
     *   1. 遍历指定组的每个 meshlet 的所有三角形
     *   2. 使用全局顶点索引（通过 meshletVertices 映射）去重顶点
     *   3. 输出紧凑的 position + index buffer
     *
     * 组边界识别：
     *   合并后，跨组的共享边变成只被 1 个三角形引用的 border edge。
     *   后续对 outGroups[g] 调用 meshopt_simplify(SimplifyLockBorder)
     *   即可自动锁定组边界，只化简内部区域。
     */
    void MergeGroups(
        const struct MeshletFileData&    inData,
        std::vector<MergedGroup>&        outGroups);

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
    uint32_t mNumPartitions    = 0;    // 0 = auto sqrt(|V|)
};

NS_RENDERSYSTEM_END

#endif // GNXENGINE_MESHLET_BUILDER_INCLUDE_H
