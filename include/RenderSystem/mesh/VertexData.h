//
//  VertexData.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNX_ENGINE_RENDERSYSTEM_VERTEX_DATA_INCLD
#define GNX_ENGINE_RENDERSYSTEM_VERTEX_DATA_INCLD

#include <stdio.h>
#include "RenderSystem/RSDefine.h"
#include "RenderCore/RenderDefine.h"
#include "BaseLib/StrideIterator.h"

NS_RENDERSYSTEM_BEGIN

using namespace baselib;

// Ordering is like this so it matches valid D3D9 FVF layouts
// Range must fit into an SInt8
enum ShaderChannel
{
    kShaderChannelNone = -1,
    kShaderChannelPosition = 0,    // position (vector3)
    kShaderChannelNormal,        // Normal (vector3)
    kShaderChannelColor,        // Vertex color
    kShaderChannelTexCoord0,    // UV set 0 (vector2)
    kShaderChannelTexCoord1,    // UV set 1 (vector2)
    kShaderChannelTangent,        // Tangent (vector4)
    kShaderChannelBoneIndex,      // BoneIndex (vec4ui)
    kShaderChannelWeight,          // Weight (vector4)
    kShaderChannelCount,            // Keep this last!
};

// 骨骼动画相关的
struct BoneIndexInfo
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
};

//子网格的信息
struct SubMeshInfo
{
    uint32_t firstIndex;   //第一个绘制的索引，即索引个数的偏移
    uint32_t indexCount;    //索引的个数，即绘制顶点的个数
    PrimitiveMode topology;
    uint32_t vertexCount;   //顶点的个数
};

//顶点数据的通道信息
typedef struct ChannelInfo
{
    uint32_t offset;
    VertexFormat format;
    uint32_t stride;

    enum { kInvalidDimension = 0 };

    // We use default constructors instead of memset()
    ChannelInfo() : offset(0), format(VertexFormatInvalid) {}

    uint32_t CalcOffset() const { return offset; }
    uint32_t CalcStride() const { return stride; }
    bool IsValid() const { return (VertexFormatInvalid != format); }
    void Reset() { *this = ChannelInfo(); }

    bool operator == (const ChannelInfo& rhs) const { return (offset == rhs.offset) && (format == rhs.format); }
    bool operator != (const ChannelInfo& rhs) const { return !(*this == rhs); }
    
} ChannelInfoArray[kShaderChannelCount];

//顶点数据的类用于存储所有的顶点属性
class VertexData
{
public:
    VertexData();
    
    VertexData(uint32_t vertexCount, uint32_t vertexSize);
    
    ~VertexData();
    
    bool Resize(uint32_t vertexCount, uint32_t vertexSize);
    
    bool HasChannel(ShaderChannel shaderChannelIndex) const
    {
        //assert((mChannels[shaderChannelIndex].format != VertexFormatInvalid));
        return mChannels[shaderChannelIndex].format != VertexFormatInvalid;
    }
    
    const ChannelInfo* GetChannels() const { return mChannels; }
    ChannelInfo* GetChannels() { return mChannels; }
    const ChannelInfo& GetChannel(int index) const { return mChannels[index]; }
    
    uint32_t GetDataSize() const { return mDataSize; }
    uint32_t GetVertexSize() const { return mVertexSize; }
    uint32_t GetVertexCount() const { return mVertexCount; }
    uint32_t GetChannelOffset(uint32_t channel) const { return mChannels[channel].CalcOffset(); }
    uint32_t GetChannelStride(uint32_t channel) const { return mChannels[channel].CalcStride(); }
    uint8_t* GetDataPtr() const { return mData; }
    
    template<class T>
    StrideIterator<T> MakeStrideIterator(ShaderChannel shaderChannelIndex) const
    {
        assert(shaderChannelIndex < kShaderChannelCount);
        void* p = mData + GetChannelOffset(shaderChannelIndex);
        return HasChannel(shaderChannelIndex) ? StrideIterator<T>(p, GetChannelStride(shaderChannelIndex)) : StrideIterator<T>(NULL, GetChannelStride(shaderChannelIndex));
    }
    
    template<class T>
    StrideIterator<T> MakeEndIterator(ShaderChannel shaderChannelIndex) const
    {
        T* end = GetEndPointer<T> (shaderChannelIndex);
        return StrideIterator<T>(end, GetChannelStride(shaderChannelIndex));
    }
    
    template<class T>
    T* GetEndPointer(ShaderChannel shaderChannelIndex) const
    {
        assert(shaderChannelIndex < kShaderChannelCount);
        void* p = HasChannel(shaderChannelIndex) ? (mData + GetChannelOffset(shaderChannelIndex) + mVertexCount * GetChannelStride(shaderChannelIndex)) : NULL;
        return (T*)p;
    }
    
private:
    ChannelInfoArray mChannels;
    
    uint8_t *mData = nullptr;      // 数据指针
    uint32_t mDataSize = 0;       //数据的大小
    uint32_t mVertexCount = 0;    //顶点的个数
    uint32_t mVertexSize = 0;     //每一个顶点的大小
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_RENDERSYSTEM_VERTEX_DATA_INCLD */
