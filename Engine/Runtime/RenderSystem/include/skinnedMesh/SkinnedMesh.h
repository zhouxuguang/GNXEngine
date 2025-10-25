//
//  SkinnedMesh.h 蒙皮网格的管理
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/27.
//

#ifndef GNX_ENGINE_SKINNED_MESH_HJFJSDJ_INCLUDE_H
#define GNX_ENGINE_SKINNED_MESH_HJFJSDJ_INCLUDE_H

#include "RenderSystem/RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector2.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Vector4.h"
//#include "MathUtil/SimdMath.h"
#include "RenderCore/RenderDevice.h"
#include "../mesh/VertexData.h"
#include "animation/Skeleton.h"
#include "animation/AnimationPose.h"

NS_RENDERSYSTEM_BEGIN

USING_NS_MATHUTIL

//SkinnedMesh，一个SkinnedMesh中可能有多个submesh
class SkinnedMesh
{
public:
    SkinnedMesh();
    
    ~SkinnedMesh();
    
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
    
    // 获得内部的顶点数据
    const VertexData& GetVertexData() const { return mVertexData; }
    VertexData& GetVertexData() { return mVertexData; }
    
    const VertexData& GetSkinnedVertexData() const { return mSkinnedVertexData; }
    VertexData& GetSkinnedVertexData() { return mSkinnedVertexData; }
    
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
    
    // 蒙皮网格特有的
    StrideIterator<BoneIndexInfo> GetBoneIndexBegin() const { return mVertexData.MakeStrideIterator<BoneIndexInfo>(kShaderChannelBoneIndex); }
    StrideIterator<BoneIndexInfo> GetBoneIndexEnd() const { return mVertexData.MakeEndIterator<BoneIndexInfo>(kShaderChannelBoneIndex); }
    
    StrideIterator<Vector4f> GetBoneWeightBegin() const { return mVertexData.MakeStrideIterator<Vector4f>(kShaderChannelWeight); }
    StrideIterator<Vector4f> GetBoneWeightEnd() const { return mVertexData.MakeEndIterator<Vector4f>(kShaderChannelWeight); }
    
    void SetPositions(Vector4f const* data, size_t count);
    void SetNormals(Vector4f const* data, size_t count);
    void SetTangents(Vector4f const* data, size_t count);
    void SetUv(int uvIndex, Vector2f const* data, size_t count);
    void SetColors(uint32_t const* data, size_t count);
    void SetBoneIndexs(BoneIndexInfo const* data, size_t count);
    void SetBoneWeights(Vector4f const* data, size_t count);
    
    void SetIndices(uint32_t const* data, size_t count);
    
    // CPU蒙皮
    void CPUSkin(Skeleton& skeleton, AnimationPose& pose);
    
    // GPU蒙皮
    void GPUSkin(Skeleton& skeleton, AnimationPose& pose);
    
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
    
    UniformBufferPtr GetSkinnedMatrixBuffer() const
    {
        return mSkinnedMatrixBuffer;
    }
    
private:
    VertexData mVertexData;
    VertexData mSkinnedVertexData;
    std::vector<uint32_t> mIndices;   //绘制的索引
    
    std::vector<SubMeshInfo> mSubMeshInfos; 
    
    VertexBufferPtr mVertexBuffer = nullptr;
    IndexBufferPtr mIndexBuffer = nullptr;
    TextureSamplerPtr mTextureSampler = nullptr;
    UniformBufferPtr mSkinnedMatrixBuffer = nullptr;
};

typedef std::shared_ptr<SkinnedMesh> SkinnedMeshPtr;

NS_RENDERSYSTEM_END

#endif
