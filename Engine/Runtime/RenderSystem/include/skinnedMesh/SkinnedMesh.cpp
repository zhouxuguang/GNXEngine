#include "SkinnedMesh.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

SkinnedMesh::SkinnedMesh()
{
}

SkinnedMesh::~SkinnedMesh()
{
    LOG_INFO("Mesh::~Mesh()");
}

void SkinnedMesh::SetPositions(Vector3f const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetPositionBegin());
}

void SkinnedMesh::SetNormals(Vector3f const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetNormalBegin());
}

void SkinnedMesh::SetTangents(Vector4f const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetTangentBegin());
}

void SkinnedMesh::SetUv(int uvIndex, Vector2f const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetUvBegin(uvIndex));
}

void SkinnedMesh::SetColors(uint32_t const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetColorBegin());
}

void SkinnedMesh::SetBoneIndexs(BoneIndexInfo const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetBoneIndexBegin());
}

void SkinnedMesh::SetBoneWeights(Vector4f const* data, size_t count)
{
    if (count > std::numeric_limits<uint16_t>::max())
    {
        LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }

    size_t prevCount = GetVertexCount();

    // Make sure we'll not be overrunning the buffer
    if (GetVertexCount() < count)
        count = GetVertexCount();

    strided_copy(data, data + count, GetBoneWeightBegin());
}

void SkinnedMesh::SetIndices(uint32_t const* data, size_t count)
{
    mIndices.resize(count);
    std::copy(data, data + count, mIndices.begin());
}

void SkinnedMesh::CPUSkin(Skeleton& skeleton, AnimationPose& pose)
{
    unsigned int numVerts = GetVertexCount();
    if (numVerts == 0) { return; }

    std::vector<Matrix4x4f> posePalette;
    pose.GetMatrixPalette(posePalette);   //pose矩阵Palette
    std::vector<Matrix4x4f> invPosePalette = skeleton.GetInvBindPoses(); //逆绑定矩阵Palette
    //invPosePalette = posePalette;
    //skeleton.GetBindPose().GetMatrixPalette(posePalette);
    
    StrideIterator<BoneIndexInfo> boneIndex = GetBoneIndexBegin();
    StrideIterator<Vector4f> boneWeight = GetBoneWeightBegin();
    StrideIterator<Vector3f> position = GetPositionBegin();
    StrideIterator<Vector3f> normal = GetNormalBegin();
    
    StrideIterator<Vector3f> skinnedPosition = mSkinnedVertexData.MakeStrideIterator<Vector3f>(kShaderChannelPosition);
    StrideIterator<Vector3f> skinnedNormal = mSkinnedVertexData.MakeStrideIterator<Vector3f>(kShaderChannelNormal);

    for (unsigned int i = 0; i < numVerts; ++i) 
    {
        const BoneIndexInfo& j = boneIndex[i];
        const Vector4f& w = boneWeight[i];

        //分别计算每一个骨骼的影响矩阵
        Matrix4x4f m0 = (posePalette[j.x] * invPosePalette[j.x]) * w.x;
        Matrix4x4f m1 = (posePalette[j.y] * invPosePalette[j.y]) * w.y;
        Matrix4x4f m2 = (posePalette[j.z] * invPosePalette[j.z]) * w.z;
        Matrix4x4f m3 = (posePalette[j.w] * invPosePalette[j.w]) * w.w;

        //计算最后的蒙皮矩阵
        Matrix4x4f skin = m0 + m1 + m2 + m3;
        //skin = Matrix4x4f::IDENTITY;

        Vector4f tranformedPoint = skin * Vector4f(position[i].x, position[i].y, position[i].z, 1.0);
        skinnedPosition[i] = Vector3f(tranformedPoint.x / tranformedPoint.w,
                                              tranformedPoint.y / tranformedPoint.w,
                                              tranformedPoint.z / tranformedPoint.w);
        Vector3f transformedNormal = skin * Vector3f(normal[i].x, normal[i].y, normal[i].z);
        skinnedNormal[i] = Vector3f(transformedNormal.x, transformedNormal.y, transformedNormal.z);
    }
    
    // 更新gpu buffer
    void *data = mVertexBuffer->MapBufferData();
    memcpy(data, mSkinnedVertexData.GetDataPtr(), mSkinnedVertexData.GetDataSize());
    mVertexBuffer->UnmapBufferData(data);
}

struct SkinnedMatrix
{
    Matrix4x4f pose[120];
    //Matrix4x4f invBindPose[40];
};

void SkinnedMesh::GPUSkin(Skeleton& skeleton, AnimationPose& pose)
{
    std::vector<Matrix4x4f> posePalette;
    pose.GetMatrixPalette(posePalette);   //pose矩阵Palette
    std::vector<Matrix4x4f> invPosePalette = skeleton.GetInvBindPoses(); //逆绑定矩阵Palette
    assert(posePalette.size() == invPosePalette.size());
    
    // 这里计算出最终的矩阵
    for (int i = 0; i < posePalette.size(); i ++)
    {
        posePalette[i] = posePalette[i] * invPosePalette[i];
    }
    
    mSkinnedMatrixBuffer->SetData(posePalette.data(), 0, (uint32_t)posePalette.size() * sizeof(Matrix4x4f));
    //mSkinnedMatrixBuffer->setData(invPosePalette.data(), offsetof(SkinnedMatrix, invBindPose), (uint32_t)invPosePalette.size() * sizeof(Matrix4x4f));
}

void SkinnedMesh::AddSubMeshInfo(const SubMeshInfo& subMeshInfo)
{
    mSubMeshInfos.push_back(subMeshInfo);
}

void SkinnedMesh::SetUpBuffer()
{
    mVertexBuffer = GetRenderDevice()->CreateVertexBufferWithBytes(mVertexData.GetDataPtr(),
            (uint32_t)mVertexData.GetDataSize(), StorageModeShared);
    
    mIndexBuffer = GetRenderDevice()->CreateIndexBufferWithBytes(mIndices.data(),
            (uint32_t)mIndices.size() * sizeof(uint32_t), IndexType_UInt);
    
    SamplerDesc samplerDescriptor;
    samplerDescriptor.filterMip = MIN_LINEAR_MIPMAP_LINEAR;
    mTextureSampler = GetRenderDevice()->CreateSamplerWithDescriptor(samplerDescriptor);
    
    mSkinnedMatrixBuffer = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(SkinnedMatrix));
}

NS_RENDERSYSTEM_END
