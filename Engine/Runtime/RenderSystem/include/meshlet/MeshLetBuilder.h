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
