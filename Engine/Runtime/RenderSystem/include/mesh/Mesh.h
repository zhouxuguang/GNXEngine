//
//  Mesh.h 静态mesh的管理
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/27.
//

#ifndef GNX_ENGINE_MESH_HJFJSDJ_INCLUDE_H
#define GNX_ENGINE_MESH_HJFJSDJ_INCLUDE_H

#include "../RSDefine.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/MathUtil/include/SimdMath.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "VertexData.h"


NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//Mesh，一个mesh中可能有多个submesh
class RENDERSYSTEM_API Mesh
{
public:
    Mesh();
    
    ~Mesh();
    
    //是否有顶点数据
    bool HasVertexData() const { return mVertexData.GetDataPtr () != NULL; }
    bool HasChannel(ShaderChannel shaderChannelIndex) const {return mVertexData.HasChannel(shaderChannelIndex);}
    
    uint32_t GetVertexDataSize() const { return mVertexData.GetDataSize(); }
    uint32_t GetVertexSize() const { return mVertexData.GetVertexSize(); }
    uint32_t GetVertexCount() const {return mVertexData.GetVertexCount();}
    
    uint32_t GetSubMeshCount() const
    {
        return (uint32_t)mSubMeshInfos.size();
    }
    
    const VertexData& GetVertexData() const { return mVertexData; }
    VertexData& GetVertexData() { return mVertexData; }
    
    StrideIterator<Vector3f> GetPositionBegin() const { return mVertexData.MakeStrideIterator<Vector3f>(kShaderChannelPosition);}
    StrideIterator<Vector3f> GetPositionEnd() const { return mVertexData.MakeEndIterator<Vector3f>(kShaderChannelPosition); }

    template<class T>
    StrideIterator<T> GetNormalBegin() const { return mVertexData.MakeStrideIterator<T>(kShaderChannelNormal); }
    template<class T>
    StrideIterator<T> GetNormalEnd() const { return mVertexData.MakeEndIterator<T>(kShaderChannelNormal); }

    StrideIterator<uint32_t> GetColorBegin() const { return mVertexData.MakeStrideIterator<uint32_t>(kShaderChannelColor); }
    StrideIterator<uint32_t> GetColorEnd() const { return mVertexData.MakeEndIterator<uint32_t>(kShaderChannelColor); }

    StrideIterator<Vector2f> GetUvBegin(int uvIndex = 0) const
    {
        return mVertexData.MakeStrideIterator<Vector2f>((ShaderChannel)(kShaderChannelTexCoord0 + uvIndex));
    }
    StrideIterator<Vector2f> GetUvEnd(int uvIndex = 0) const
    {
        return mVertexData.MakeEndIterator<Vector2f>((ShaderChannel)(kShaderChannelTexCoord0 + uvIndex));
    }

    template<class T>
    StrideIterator<T> GetTangentBegin() const { return mVertexData.MakeStrideIterator<T>(kShaderChannelTangent); }
    template<class T>
    StrideIterator<T> GetTangentEnd() const { return mVertexData.MakeEndIterator<T>(kShaderChannelTangent); }
    
    void SetPositions(Vector3f const* data, size_t count);

    template<class T>
    void SetNormals(T const* data, size_t count)
    {
		if (count > std::numeric_limits<uint32_t>::max())
		{
			LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
			return;
		}

		size_t prevCount = GetVertexCount();

		// Make sure we'll not be overrunning the buffer
		if (GetVertexCount() < count)
			count = GetVertexCount();

		strided_copy<T>(data, data + count, GetNormalBegin<T>());
    }

    template<class T>
    void SetTangents(T const* data, size_t count)
    {
		if (count > std::numeric_limits<uint32_t>::max())
		{
			LOG_INFO("Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
			return;
		}

		size_t prevCount = GetVertexCount();

		// Make sure we'll not be overrunning the buffer
		if (GetVertexCount() < count)
			count = GetVertexCount();

        strided_copy(data, data + count, GetTangentBegin<T>());
    }

    void SetUv(int uvIndex, Vector2f const* data, size_t count);
    void SetColors(uint32_t const* data, size_t count);
    
    void SetIndices(uint32_t const* data, size_t count);

    const std::vector<uint32_t>& GetIndices() const
    {
        return mIndices;
    }
    
    void AddSubMeshInfo(const SubMeshInfo& subMeshInfo);
    
    const SubMeshInfo& GetSubMeshInfo(int index) const
    {
        return mSubMeshInfos[index];
    }
    
    void SetUpBuffer();

    // Update index buffer (for dynamic LOD terrain, etc.)
    // Replaces the index data and recreates the GPU index buffer.
    // Also updates the first SubMeshInfo's indexCount.
    void UpdateIndices(const uint32_t* data, size_t count);

    VertexBufferPtr GetVertexBuffer() const
    {
        return mVertexBuffer;
    }
    
    IndexBufferPtr GetIndexBuffer() const
    {
        return mIndexBuffer;
    }
    
    TextureSamplerPtr GetSampler() const
    {
        return mTextureSampler;
    }
    
private:
    VertexData mVertexData;
    std::vector<uint32_t> mIndices;   //绘制的索引
    
    std::vector<SubMeshInfo> mSubMeshInfos; 
    
    VertexBufferPtr mVertexBuffer = nullptr;
    IndexBufferPtr mIndexBuffer = nullptr;
    TextureSamplerPtr mTextureSampler = nullptr;
};

typedef std::shared_ptr<Mesh> MeshPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_MESH_HJFJSDJ_INCLUDE_H */
