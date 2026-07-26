//
//  MeshLetBuilder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2024/6/14.
//

#include "meshlet/MeshLetBuilder.h"
#include "meshlet/MeshLetFile.h"
#include "meshoptimizer.h"
#include <metis.h>

#include "Runtime/BaseLib/include/LogService.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <utility>

NS_RENDERSYSTEM_BEGIN

// 无序边类型：first ≤ second 恒成立
using Edge = std::pair<uint32_t, uint32_t>;

// =======================================================================
// MeshletBuilder 实现
// =======================================================================

MeshletBuilder::MeshletBuilder()
    : mMaxVertices(kMeshletMaxVertices)
    , mMaxTriangles(kMeshletMaxTriangles)
    , mTargetError(0.01f)
    , mMaxLodLevels(20)
    , mMinTriangles(kMeshletMaxTriangles)
    , mLodSimplifyRatio(0.5f)
{
}

MeshletBuilder::~MeshletBuilder()
{
}

bool MeshletBuilder::Build(
    const float*    positions,
    size_t          vertexCount,
    const uint32_t* indices,
    size_t          indexCount,
    MeshletFileData& outData)
{
    if (!positions || vertexCount == 0 || !indices || indexCount == 0)
        return false;

    // --- 1. 保存原始顶点位置 ---
    outData.vertexPositions.assign(positions, positions + vertexCount * 3);

    // --- 2. 构建原始网格的 meshlet（单 LOD） ---
    std::vector<Meshlet>   lodMeshlets;
    std::vector<uint32_t>  lodMeshletVerts;
    std::vector<uint32_t>  lodMeshletTris;
    BuildSingleLod(indices, indexCount,
                   outData.vertexPositions.data(), vertexCount,
                   lodMeshlets, lodMeshletVerts, lodMeshletTris);

    outData.lodMeshletOffsets.push_back(0);
    outData.lodMeshletCounts.push_back(static_cast<uint32_t>(lodMeshlets.size()));

    outData.meshlets = std::move(lodMeshlets);
    outData.meshletVertices = std::move(lodMeshletVerts);
    outData.meshletTriangles = std::move(lodMeshletTris);
    outData.lodCount = 1;

    // ---- LOD 0 叶节点初始化：lodError=0, 父信息=自身 ----
    for (auto& m : outData.meshlets)
    {
        m.lodError = 0.0f;
        m.parentLodError = 0.0f;
        m.parentBoundingSphere = m.boundingSphere;
    }

    LOG_INFO("Original mesh: %zu triangles -> %zu meshlets",
        indexCount / 3, outData.meshlets.size());

    // --- 3. 构建连通图并用 METIS 分区 ---
    if (outData.meshlets.size() > 1)
    {
        std::vector<metis_idx_t> xadj, adjncy, adjwgt;
        BuildConnectivityGraph(outData.meshlets,
                               outData.meshletVertices,
                               outData.meshletTriangles,
                               indices,
                               indexCount,
                               xadj, adjncy, adjwgt);

        // 分区数：按每组目标 meshlet 数自适应
        metis_idx_t numParts;
        if (mNumPartitions > 0)
            // mNumPartitions = 每组目标 meshlet 数, 实际分区 = ceil(N / target), 至少 2
            numParts = std::max<metis_idx_t>(2,
                static_cast<metis_idx_t>(outData.meshlets.size()) / static_cast<metis_idx_t>(mNumPartitions));
        else
            numParts = std::max<metis_idx_t>(1,
                static_cast<metis_idx_t>(std::sqrt(static_cast<double>(outData.meshlets.size()))));

        std::vector<metis_idx_t> part(outData.meshlets.size());
        if (PartitionWithMetis(static_cast<metis_idx_t>(outData.meshlets.size()),
                               xadj, adjncy, adjwgt, numParts, part))
        {
            outData.numPartitions = static_cast<uint32_t>(numParts);
            outData.meshletPartitions.resize(outData.meshlets.size());
            for (size_t i = 0; i < outData.meshlets.size(); ++i)
            {
                outData.meshletPartitions[i] = static_cast<uint32_t>(part[i]);
            }

            LOG_INFO("METIS partition completed: %lld groups for %lld meshlets.",
                static_cast<long long>(numParts),
                static_cast<long long>(outData.meshlets.size()));
        }
    }

    return true;
}

// =======================================================================
// BuildSingleLod
// =======================================================================

void MeshletBuilder::BuildSingleLod(
    const uint32_t* indices,
    size_t          indexCount,
    const float*    positions,
    size_t          vertexCount,
    std::vector<Meshlet>&    outMeshlets,
    std::vector<uint32_t>&   outVertices,
    std::vector<uint32_t>&   outTriangles)
{
    BuildMeshlets(indices, indexCount,
                  positions, vertexCount, sizeof(float) * 3,
                  outMeshlets, outVertices, outTriangles);
}

// =======================================================================
// BuildConnectivityGraph
//
// 算法详解：
//
//   meshlet 是由一组三角形组成的子网格。两个 meshlet "共享边界"
//   指的是它们包含同一条边（由两个全局顶点索引定义的无序对）。
//
//   步骤：
//   1. 遍历每个 meshlet 的所有三角形，对每个三角形提取 3 条无序边。
//      unorderedEdge = (min(vi, vj), max(vi, vj))
//      边中的 vi/vj 是全局顶点索引（通过 meshletVertices 映射得到）。
//
//   2. 建立 edge -> set<meshletId> 映射。
//      对于 meshlet i 中每个三角形的 3 条边，
//      将 meshletId = i 插入 edgeMap[edge] 的集合中。
//
//   3. 在 edgeMap 中，对每个出现在 >= 2 个 meshlet 中的边，
//      将该边的每对 meshlet (a, b) 的共享计数 +1。
//      使用 map<(a,b), count> 聚合。
//
//   4. 将聚合结果转换为 METIS CSR 格式：
//      xadj[i]     = 顶点 i 的邻接表起始位置
//      adjncy[...] = 邻居顶点编号
//      adjwgt[...] = 边权重（共享边数量）
//
//   图示：
//
//   Meshlet A                 Meshlet B
//     v0---v1                  v1---v3
//     |  /|                    |  /|
//     | / |   edge (v1,v2)     | / |
//     |/  |   <============>   |/  |
//     v5--v2                   v2--v4
//
//   共享边 (v1,v2) 同时出现在 A 和 B 中
//     -> edgeMap[(1,2)] = {A, B}
//     -> pairWeight[(A,B)] += 1
//
// =======================================================================

void MeshletBuilder::BuildConnectivityGraph(
    const std::vector<Meshlet>&    meshlets,
    const std::vector<uint32_t>&   meshletVertices,
    const std::vector<uint32_t>&   meshletTriangles,
    const uint32_t*                globalIndices,
    size_t                         indexCount,
    std::vector<metis_idx_t>&      outXadj,
    std::vector<metis_idx_t>&      outAdjncy,
    std::vector<metis_idx_t>&      outAdjwgt)
{
    const size_t meshletCount = meshlets.size();

    // ---- Step 1 & 2: 遍历所有 meshlet 的三角形，提取共享边 ----

    // edge -> set of meshlet IDs that contain this edge
    std::map<Edge, std::set<uint32_t>> edgeMap;

    for (size_t m = 0; m < meshletCount; ++m)
    {
        const Meshlet& meshlet = meshlets[m];

        for (uint32_t t = 0; t < meshlet.triangleCount; ++t)
        {
            // 从 packed uint32_t 解码 3 个 local vertex indices
            uint32_t packed = meshletTriangles[meshlet.triangleOffset + t];
            uint8_t  localIdx0 = (packed >> 0)  & 0xFF;
            uint8_t  localIdx1 = (packed >> 8)  & 0xFF;
            uint8_t  localIdx2 = (packed >> 16) & 0xFF;

            // local -> global vertex indices
            uint32_t globalIdx0 = meshletVertices[meshlet.vertexOffset + localIdx0];
            uint32_t globalIdx1 = meshletVertices[meshlet.vertexOffset + localIdx1];
            uint32_t globalIdx2 = meshletVertices[meshlet.vertexOffset + localIdx2];

            // 3 条无序边
            Edge e0 = { std::min(globalIdx0, globalIdx1), std::max(globalIdx0, globalIdx1) };
            Edge e1 = { std::min(globalIdx1, globalIdx2), std::max(globalIdx1, globalIdx2) };
            Edge e2 = { std::min(globalIdx2, globalIdx0), std::max(globalIdx2, globalIdx0) };

            edgeMap[e0].insert(static_cast<uint32_t>(m));
            edgeMap[e1].insert(static_cast<uint32_t>(m));
            edgeMap[e2].insert(static_cast<uint32_t>(m));
        }
    }

    // ---- Step 3: 聚合共享边界权重 ----

    // (meshletA, meshletB) -> shared edge count (sorted, so A < B)
    std::map<Edge, metis_idx_t> pairWeight;

    for (const auto& kv : edgeMap)
    {
        const std::set<uint32_t>& meshletSet = kv.second;
        if (meshletSet.size() < 2)
            continue; // 只属于一个 meshlet 的边不构成共享

        // 对 set 中每对 meshlet 的共享计数 +1
        for (auto itA = meshletSet.begin(); itA != meshletSet.end(); ++itA)
        {
            auto itB = itA;
            ++itB;
            for (; itB != meshletSet.end(); ++itB)
            {
                uint32_t a = *itA;
                uint32_t b = *itB;
                Edge key = { std::min(a, b), std::max(a, b) };
                pairWeight[key] += 1;
            }
        }
    }

    // ---- Step 4: 转换为 METIS CSR 格式 ----

    // METIS 要求无向图，每条边存两次 (a->b 和 b->a)

    // 先计算每个顶点的度
    std::vector<metis_idx_t> degree(meshletCount, 0);
    for (const auto& kv : pairWeight)
    {
        uint32_t a = kv.first.first;
        uint32_t b = kv.first.second;
        degree[a]++;
        degree[b]++;
    }

    // 构建 xadj
    outXadj.resize(meshletCount + 1);
    outXadj[0] = 0;
    for (size_t i = 0; i < meshletCount; ++i)
    {
        outXadj[i + 1] = outXadj[i] + degree[i];
    }

    const metis_idx_t totalEdges = outXadj[meshletCount];
    outAdjncy.resize(static_cast<size_t>(totalEdges));
    outAdjwgt.resize(static_cast<size_t>(totalEdges));

    // 填充邻接表（用 xadj 副本追踪当前写入位置）
    std::vector<metis_idx_t> cursor = outXadj;
    for (const auto& kv : pairWeight)
    {
        uint32_t a = kv.first.first;
        uint32_t b = kv.first.second;
        metis_idx_t w = kv.second;

        // a -> b
        metis_idx_t posA = cursor[a]++;
        outAdjncy[posA] = static_cast<metis_idx_t>(b);
        outAdjwgt[posA] = w;

        // b -> a
        metis_idx_t posB = cursor[b]++;
        outAdjncy[posB] = static_cast<metis_idx_t>(a);
        outAdjwgt[posB] = w;
    }

    LOG_INFO("Connectivity graph: %zu nodes, %zu unique edges, %lld directed edges",
        meshletCount, pairWeight.size(), static_cast<long long>(totalEdges));
}

// =======================================================================
// PartitionWithMetis
// =======================================================================

bool MeshletBuilder::PartitionWithMetis(
    metis_idx_t                     meshletCount,
    const std::vector<metis_idx_t>& xadj,
    const std::vector<metis_idx_t>& adjncy,
    const std::vector<metis_idx_t>& adjwgt,
    metis_idx_t                     numParts,
    std::vector<metis_idx_t>&       outPart)
{
    if (meshletCount == 0 || numParts <= 0)
        return false;

    outPart.resize(static_cast<size_t>(meshletCount));

    // METIS 要求参数为非 const 指针 (metis_idx_t == int64_t == idx_t)
    auto nvtxs   = static_cast<idx_t>(meshletCount);
    auto ncon    = static_cast<idx_t>(1);
    auto nparts  = static_cast<idx_t>(numParts);
    idx_t edgecut = 0;

    // 选项数组
    std::vector<idx_t> options(METIS_NOPTIONS);
    options[METIS_OPTION_OBJTYPE]   = METIS_OBJTYPE_CUT;
	options[METIS_OPTION_CCORDER]   = 1; // identify connected components first
	options[METIS_OPTION_NUMBERING] = 0;
    options[METIS_OPTION_UFACTOR]   = 5;
    METIS_SetDefaultOptions(options.data());

    int result = METIS_PartGraphKway(
        &nvtxs, &ncon,
        const_cast<idx_t*>(xadj.data()),
        const_cast<idx_t*>(adjncy.data()),
        nullptr, nullptr,
        const_cast<idx_t*>(adjwgt.data()),
        &nparts,
        nullptr, nullptr,
        options.data(),
        &edgecut,
        outPart.data());

    LOG_INFO("METIS_PartGraphKway result=%d, edgecut=%lld (meshletCount=%lld, numParts=%lld)",
        result, static_cast<long long>(edgecut),
        static_cast<long long>(meshletCount),
        static_cast<long long>(numParts));

    return (result == METIS_OK);
}

// =======================================================================
// MergeGroups
// =======================================================================

void MeshletBuilder::MergeGroups(
    const MeshletFileData&   inData,
    size_t                   meshletStart,
    size_t                   meshletCount,
    std::vector<MergedGroup>& outGroups)
{
    const uint32_t numGroups = inData.numPartitions;
    if (numGroups == 0 || inData.meshletPartitions.empty())
        return;

    outGroups.resize(numGroups);

    const size_t endIdx = meshletStart + meshletCount;

    for (uint32_t g = 0; g < numGroups; ++g)
    {
        MergedGroup& group = outGroups[g];
        group.groupId = g;

        // 只遍历指定范围内的 meshlet
        for (size_t m = meshletStart; m < endIdx; ++m)
        {
            if (inData.meshletPartitions[m] != g)
                continue;

            const Meshlet& meshlet = inData.meshlets[m];

            for (uint32_t t = 0; t < meshlet.triangleCount; ++t)
            {
                // 解码 packed triangle → 3 个 local 顶点索引
                uint32_t packed    = inData.meshletTriangles[meshlet.triangleOffset + t];
                uint32_t localIdx0 = (packed >>  0) & 0xFF;
                uint32_t localIdx1 = (packed >>  8) & 0xFF;
                uint32_t localIdx2 = (packed >> 16) & 0xFF;

                // local → global 顶点索引，直接存入（无需去重——重复出现的顶点就是共享顶点）
                uint32_t globalIdx0 = inData.meshletVertices[meshlet.vertexOffset + localIdx0];
                uint32_t globalIdx1 = inData.meshletVertices[meshlet.vertexOffset + localIdx1];
                uint32_t globalIdx2 = inData.meshletVertices[meshlet.vertexOffset + localIdx2];

                group.triangleIndices.push_back(globalIdx0);
                group.triangleIndices.push_back(globalIdx1);
                group.triangleIndices.push_back(globalIdx2);
            }
        }

        LOG_INFO("Merged group %u: %zu triangles (global indices)",
            g,
            group.triangleIndices.size() / 3);
    }
}

// =======================================================================
// BuildNextLOD
// =======================================================================

bool MeshletBuilder::BuildNextLOD(
    MeshletFileData& inOutData)
{
    //return false;
    const size_t currentMeshletCount = inOutData.meshlets.size();

    // 获取最后一级 LOD 的 meshlet 数量
    size_t lastLodCount = currentMeshletCount;
    if (inOutData.lodMeshletOffsets.size() > 0)
        lastLodCount = currentMeshletCount - inOutData.lodMeshletOffsets.back();

    // 停止条件：最后一级 LOD 的 meshlet 数量低于阈值
    if (lastLodCount < mMinMeshletsForPartition || currentMeshletCount < 2)
    {
        LOG_INFO("BuildNextLOD: only %zu meshlets in last LOD (< %u), stop.",
            lastLodCount, mMinMeshletsForPartition);
        return false;
    }

    // --- Step 1: 合并同组 meshlet ---
    // 找到当前 LOD 的 meshlet slice：最后一个 LOD 的起始偏移
    size_t lodStart = currentMeshletCount;  // 默认：所有 meshlet
    if (inOutData.lodMeshletOffsets.size() > 0)
        lodStart = inOutData.lodMeshletOffsets.back();
    size_t lodMeshletCount = currentMeshletCount - lodStart;

    std::vector<MergedGroup> mergedGroups;
    MergeGroups(inOutData, lodStart, lodMeshletCount, mergedGroups);

    const size_t numGroups = mergedGroups.size();
    if (numGroups == 0) return false;

    // --- Step 2: 每组合并后简化，再重新切 meshlet ---
    const uint32_t globalVtxCount = inOutData.GetVertexCount();
    const float*   globalPositions = inOutData.vertexPositions.data();

    // 收集 LOD N+1 的所有 meshlet 数据
    std::vector<Meshlet>    nextLodMeshlets;
    std::vector<uint32_t>   nextLodVertices;
    std::vector<uint32_t>   nextLodTriangles;

    for (size_t g = 0; g < numGroups; ++g)
    {
        MergedGroup& group = mergedGroups[g];
        if (group.triangleIndices.size() < 3)
            continue;

        // ---- 算法 4.4 Step 1-8: 计算 group 的 parent LOD 信息 ----
        // 遍历该 group 所有子 meshlet，合并包围球 + 取最大 lodError
        float              groupParentLodError = 0.0f;
        mathutil::Vector4f groupParentBounds = {0, 0, 0, 0};
        {
            // 计算所有子包围球的中心平均值作为合并球心
            mathutil::Vector3f centerSum = {0, 0, 0};
            size_t childCount = 0;
            for (size_t ci = lodStart; ci < currentMeshletCount; ++ci)
            {
                if (inOutData.meshletPartitions[ci] != g) continue;
                const Meshlet& child = inOutData.meshlets[ci];
                groupParentLodError = std::max(groupParentLodError, child.lodError);
                centerSum.x += child.boundingSphere.x;
                centerSum.y += child.boundingSphere.y;
                centerSum.z += child.boundingSphere.z;
                childCount++;
            }
            if (childCount > 0)
            {
                centerSum.x /= static_cast<float>(childCount);
                centerSum.y /= static_cast<float>(childCount);
                centerSum.z /= static_cast<float>(childCount);
                // 合并包围球半径 = 离中心最远的子球外缘
                float maxDist = 0.0f;
                for (size_t ci = lodStart; ci < currentMeshletCount; ++ci)
                {
                    if (inOutData.meshletPartitions[ci] != g) continue;
                    const Meshlet& child = inOutData.meshlets[ci];
                    float dx = child.boundingSphere.x - centerSum.x;
                    float dy = child.boundingSphere.y - centerSum.y;
                    float dz = child.boundingSphere.z - centerSum.z;
                    float dist = std::sqrt(dx*dx + dy*dy + dz*dz) + child.boundingSphere.w;
                    maxDist = std::max(maxDist, dist);
                }
                groupParentBounds = mathutil::Vector4f(centerSum.x, centerSum.y, centerSum.z, maxDist);
            }
        }

        // 简化目标：保留 mLodSimplifyRatio 比例的三角形
        size_t targetIdxCount = static_cast<size_t>(
            group.triangleIndices.size() * mLodSimplifyRatio);
        if (targetIdxCount < 3) targetIdxCount = 3;

        float error = 0.0f;
        std::vector<uint32_t> simplifiedGlobal = group.Simplify(
            globalPositions, globalVtxCount,
            targetIdxCount, mTargetError, &error);

        if (simplifiedGlobal.size() < 3) continue;

        // 在简化后的 group 内部重新切 meshlet
        std::vector<Meshlet>    groupMeshlets;
        std::vector<uint32_t>   groupVertices;
        std::vector<uint32_t>   groupTriangles;
        BuildMeshlets(
            simplifiedGlobal.data(),
            simplifiedGlobal.size(),
            globalPositions,
            globalVtxCount,
            sizeof(float) * 3,
            groupMeshlets,
            groupVertices,
            groupTriangles);

        // ---- 算法 4.4 Step 9-12: 赋 parent 值给所有父 meshlet ----
        groupParentLodError = std::max(groupParentLodError, error);  // max(子error, 本层简化error)
        for (auto& m : groupMeshlets)
        {
            m.lodError             = error;
            m.parentLodError       = groupParentLodError;
            m.parentBoundingSphere = groupParentBounds;
        }

        // 合并进 LOD N+1 数组
        for (auto& m : groupMeshlets)
        {
            m.vertexOffset   += static_cast<uint32_t>(nextLodVertices.size());
            m.triangleOffset += static_cast<uint32_t>(nextLodTriangles.size());
            nextLodMeshlets.push_back(m);
        }
        nextLodVertices.insert(nextLodVertices.end(),
            groupVertices.begin(), groupVertices.end());
        nextLodTriangles.insert(nextLodTriangles.end(),
            groupTriangles.begin(), groupTriangles.end());

        LOG_INFO("  Group %zu: %zu tri → %zu tri (err=%.4f) → %zu meshlets",
            g, group.triangleIndices.size() / 3,
            simplifiedGlobal.size() / 3,
            error,
            groupMeshlets.size());
    }

    if (nextLodMeshlets.empty())
    {
        LOG_INFO("BuildNextLOD: no meshlets generated.");
        return false;
    }

    // --- Step 3: 追加到 inOutData（更新全局偏移）---
    uint32_t lodMeshletOffset = static_cast<uint32_t>(inOutData.meshlets.size());
    uint32_t lodVertOffset    = static_cast<uint32_t>(inOutData.meshletVertices.size());
    uint32_t lodTriOffset     = static_cast<uint32_t>(inOutData.meshletTriangles.size());

    for (auto& m : nextLodMeshlets)
    {
        m.vertexOffset   += lodVertOffset;
        m.triangleOffset += lodTriOffset;
    }

    inOutData.meshlets.insert(inOutData.meshlets.end(),
        nextLodMeshlets.begin(), nextLodMeshlets.end());
    inOutData.meshletVertices.insert(inOutData.meshletVertices.end(),
        nextLodVertices.begin(), nextLodVertices.end());
    inOutData.meshletTriangles.insert(inOutData.meshletTriangles.end(),
        nextLodTriangles.begin(), nextLodTriangles.end());

    uint32_t nextLodCount = static_cast<uint32_t>(nextLodMeshlets.size());
    inOutData.lodMeshletOffsets.push_back(lodMeshletOffset);
    inOutData.lodMeshletCounts.push_back(nextLodCount);
    inOutData.lodCount++;

    // --- Step 4: 对新生成的 LOD 做 METIS 分区，为下一轮递归做准备 ---
    if (nextLodCount > 1)
    {
        // 复用 BuildConnectivityGraph，传入新 LOD 的 meshlet 切片
        // meshletVertices/meshletTriangles 已在全局 flat arrays 中
        std::vector<metis_idx_t> xadj, adjncy, adjwgt;
        BuildConnectivityGraph(
            nextLodMeshlets,
            inOutData.meshletVertices,
            inOutData.meshletTriangles,
            nullptr, 0,  // 原始三角形索引不需要（函数体内不使用）
            xadj, adjncy, adjwgt);

        // 分区数同上
        metis_idx_t numPartsForNext;
        if (mNumPartitions > 0)
            numPartsForNext = std::max<metis_idx_t>(2,
                static_cast<metis_idx_t>(nextLodCount) / static_cast<metis_idx_t>(mNumPartitions));
        else
            numPartsForNext = std::max<metis_idx_t>(2,
                static_cast<metis_idx_t>(std::sqrt(static_cast<double>(nextLodCount))));

        std::vector<metis_idx_t> newPart(nextLodCount);
        if (PartitionWithMetis(static_cast<metis_idx_t>(nextLodCount),
                               xadj, adjncy, adjwgt, numPartsForNext, newPart))
        {
            inOutData.numPartitions = static_cast<uint32_t>(numPartsForNext);
            inOutData.meshletPartitions.resize(inOutData.meshlets.size(), 0);
            for (size_t i = 0; i < newPart.size(); ++i)
            {
                inOutData.meshletPartitions[lodMeshletOffset + i] = static_cast<uint32_t>(newPart[i]);
            }

            LOG_INFO("  Re-partitioned LOD %u meshlets into %lld groups for next level.",
                inOutData.lodCount - 1, static_cast<long long>(numPartsForNext));
        }
        else
        {
            inOutData.meshletPartitions.resize(inOutData.meshlets.size(), 0);
            inOutData.numPartitions = 0;
        }
    }
    else
    {
        inOutData.meshletPartitions.resize(inOutData.meshlets.size(), 0);
        inOutData.numPartitions = 0;
    }

    LOG_INFO("BuildNextLOD: generated %u LOD %u meshlets (total LODs: %u)",
        nextLodCount, inOutData.lodCount - 1, inOutData.lodCount);

    return true;
}

NS_RENDERSYSTEM_END
