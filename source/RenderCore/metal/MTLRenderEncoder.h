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
    virtual void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline);
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    virtual void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetVertexUniformBuffer(UniformBufferPtr buffer, int index);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index);
    
    /**
     设置顶点uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);

    /**
     设置片元uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);
    
    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void DrawPrimitves(PrimitiveMode mode, int offset, int size);
    
    virtual void DrawInstancePrimitves(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount);
    
    /**
     draw funton with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset);
    
    /**
     draw function with index instance
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset 索引的偏移
     @param offset firstInstance 第一个实例的索引
     @param offset instanceCount 实例的个数
     */
    virtual void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
                                               uint32_t firstInstance, uint32_t instanceCount);
    
    /**
     设置纹理和采样器

     @param texture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index);
    
    /**
     设置立方体纹理和采样器

     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index);
    
    /**
     设置渲染纹理和采样器

     @param renderTexture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index);
    
    /**
     设置片元纹理和采样器

     @param resourceName 对应shader中的名字
     @param texture 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentTextureAndSampler(const std::string &resourceName, Texture2DPtr texture, TextureSamplerPtr sampler);

    /**
     设置片元立方体纹理和采样器

     @param resourceName 对应shader中的名字
     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentTextureCubeAndSampler(const std::string& resourceName, TextureCubePtr textureCube, TextureSamplerPtr sampler);

    /**
     设置片元渲染纹理和采样器

     @param resourceName 对应shader中的名字
     @param rendertexture 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentRenderTextureAndSampler(const std::string& resourceName, RenderTexturePtr renderTexture, TextureSamplerPtr sampler);

private:
    id <MTLRenderCommandEncoder> mRenderEncoder = nil;
    
    FrameBufferFormat mFrameBufferFormat;
    MTLGraphicsPipelinePtr mMtlGraphicsPipeline = nil;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H */
