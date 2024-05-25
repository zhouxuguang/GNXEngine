//
//  RenderEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#ifndef GNX_ENGINE_RENDER_ENCODE_INCLUDE_H
#define GNX_ENGINE_RENDER_ENCODE_INCLUDE_H

#include "RenderDefine.h"
#include "GraphicsPipeline.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"
#include "TextureSampler.h"
#include "TextureCube.h"
#include "RenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class RenderEncoder
{
public:
    RenderEncoder();
    
    virtual ~RenderEncoder();
    
    //结束渲染命令录制
    virtual void EndEncode() = 0;
    
    /**
     设置图形管线
     */
    virtual void setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline) = 0;
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    virtual void setVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index) = 0;
    
    /**
     设置顶点数据，以copy的方式直接设置，pData的大小dataLen最大为4096，即4K
     
     @param pData 数据指针
     @param dataLen 数据长度
     @param index 绑定的索引
     */
    virtual void setVertexBytes(const void* pData, size_t dataLen, int index) = 0;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void setVertexUniformBuffer(UniformBufferPtr buffer, int index) = 0;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void setFragmentUniformBuffer(UniformBufferPtr buffer, int index) = 0;
    
    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void drawPrimitves(PrimitiveMode mode, int offset, int size) = 0;
    
    /**
     draw funton with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    virtual void drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset) = 0;
    
    /**
     设置纹理和采样器

     @param texture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index) = 0;
    
    /**
     设置立方体纹理和采样器

     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index) = 0;
    
    /**
     设置渲染纹理和采样器

     @param rendertexture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentRenderTextureAndSampler(RenderTexturePtr textureCube, TextureSamplerPtr sampler, int index) = 0;
};

typedef std::shared_ptr<RenderEncoder> RenderEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_ENCODE_INCLUDE_H */
