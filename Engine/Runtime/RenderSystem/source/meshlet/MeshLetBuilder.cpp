//
//  MeshLetBuilder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2024/6/14.
//

#include "meshlet/MeshLetBuilder.h"
#include "meshlet/MeshLetFile.h"
#include "meshoptimizer.h"

NS_RENDERSYSTEM_BEGIN

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

// =======================================================================
// Build
// =======================================================================

bool MeshletBuilder::Build(
    const float*    positions,
    size_t          vertexCount,
    const uint32_t* indices,
    size_t          indexCount,
    MeshletFileData& outData)
{
    if (!positions || vertexCount == 0 || !indices || indexCount == 0)
        return false;

    // 目前先返回 false，后续实现具体逻辑
    return false;
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

NS_RENDERSYSTEM_END
