//
//  MTLRenderEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H
#define GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H

#include "MTLRenderDefine.h"
#include "RenderEncoder.h"
#include "MTLGraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLRenderEncoder : public RenderEncoder
{
public:
    MTLRenderEncoder(id<MTLRenderCommandEncoder> renderEncoder, const FrameBufferFormat& frameBufferFormat);
    
    ~MTLRenderEncoder();
    
    virtual void EndEncode();
    
    /**
     设置图形管线
     */
    virtual void setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline);
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    virtual void setVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void setVertexUniformBuffer(UniformBufferPtr buffer, int index);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void setFragmentUniformBuffer(UniformBufferPtr buffer, int index);
    
    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void drawPrimitves(PrimitiveMode mode, int offset, int size);
    
    /**
     draw funton with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    virtual void drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset);
    
    /**
     设置纹理和采样器

     @param texture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index);
    
    /**
     设置立方体纹理和采样器

     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index);
    
    /**
     设置渲染纹理和采样器

     @param renderTexture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void setFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index);

private:
    id <MTLRenderCommandEncoder> mRenderEncoder = nil;
    
    FrameBufferFormat mFrameBufferFormat;
    MTLGraphicsPipelinePtr mMtlGraphicsPipeline = nil;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H */
