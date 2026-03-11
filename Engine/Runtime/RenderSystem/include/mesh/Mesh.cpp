//
//  Mesh.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/27.
//

#include "Mesh.h"
#include "Runtime/BaseLib/include/LogService.h"

NS_RENDERSYSTEM_BEGIN

Mesh::Mesh()
{
    //
}

Mesh::~Mesh()
{
    LOG_INFO("Mesh::~Mesh()");
}

void Mesh::SetPositions(Vector4f const* data, size_t count)
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

void Mesh::SetUv(int uvIndex, Vector2f const* data, size_t count)
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

void Mesh::SetColors(uint32_t const* data, size_t count)
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

void Mesh::SetIndices(uint32_t const* data, size_t count)
{
    mIndices.resize(count);
    std::copy(data, data + count, mIndices.begin());
}

void Mesh::AddSubMeshInfo(const SubMeshInfo& subMeshInfo)
{
    mSubMeshInfos.push_back(subMeshInfo);
}

void Mesh::SetUpBuffer()
{
    mVertexBuffer = GetRenderDevice()->CreateVertexBufferWithBytes(mVertexData.GetDataPtr(),
            (uint32_t)mVertexData.GetDataSize(), StorageModePrivate);
    
    mIndexBuffer = GetRenderDevice()->CreateIndexBufferWithBytes(mIndices.data(),
            (uint32_t)mIndices.size() * sizeof(uint32_t), IndexType_UInt);
    //mIndiceCount = (uint32_t)mIndices.size();
    
    SamplerDesc samplerDescriptor;
    samplerDescriptor.filterMip = MIN_LINEAR_MIPMAP_LINEAR;
    mTextureSampler = GetRenderDevice()->CreateSamplerWithDescriptor(samplerDescriptor);
}

NS_RENDERSYSTEM_END
