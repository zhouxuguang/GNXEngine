//
//  Mesh.h 静态mesh的管理
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/27.
//

#ifndef GNX_ENGINE_MESH_HJFJSDJ_INCLUDE_H
#define GNX_ENGINE_MESH_HJFJSDJ_INCLUDE_H

#include "RenderSystem/RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector2.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Vector4.h"
#include "MathUtil/SimdMath.h"
#include "RenderCore/RenderDevice.h"
#include "VertexData.h"


NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//Mesh，一个mesh中可能有多个submesh
class Mesh
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
    
    StrideIterator<Vector4f> GetPositionBegin() const { return mVertexData.MakeStrideIterator<Vector4f>(kShaderChannelPosition);}
    StrideIterator<Vector4f> GetPositionEnd() const { return mVertexData.MakeEndIterator<Vector4f>(kShaderChannelPosition); }

    StrideIterator<Vector4f> GetNormalBegin() const { return mVertexData.MakeStrideIterator<Vector4f>(kShaderChannelNormal); }
    StrideIterator<Vector4f> GetNormalEnd() const { return mVertexData.MakeEndIterator<Vector4f>(kShaderChannelNormal); }

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

    StrideIterator<Vector4f> GetTangentBegin() const { return mVertexData.MakeStrideIterator<Vector4f>(kShaderChannelTangent); }
    StrideIterator<Vector4f> GetTangentEnd() const { return mVertexData.MakeEndIterator<Vector4f>(kShaderChannelTangent); }
    
    void SetPositions(Vector4f const* data, size_t count);
    void SetNormals(Vector4f const* data, size_t count);
    void SetTangents(Vector4f const* data, size_t count);
    void SetUv(int uvIndex, Vector2f const* data, size_t count);
    void SetColors(uint32_t const* data, size_t count);
    
    void SetIndices(uint32_t const* data, size_t count);
    
    void AddSubMeshInfo(const SubMeshInfo& subMeshInfo);
    
    const SubMeshInfo& GetSubMeshInfo(int index) const
    {
        return mSubMeshInfos[index];
    }
    
    void SetUpBuffer();
    
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
