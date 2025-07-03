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
    virtual void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline) = 0;
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    virtual void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index) = 0;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetVertexUniformBuffer(UniformBufferPtr buffer, int index) = 0;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index) = 0;

    /**
     设置顶点uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) = 0;

    /**
     设置片元uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) = 0;
    
    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void DrawPrimitves(PrimitiveMode mode, int offset, int size) = 0;

    /**
     draw function with instance
     
     @param mode mode description
     @param offset offset description
     @param size size description
     @param offset firstInstance 第一个实例的索引
     @param offset instanceCount 实例的个数
     */
    virtual void DrawInstancePrimitves(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount) = 0;
    
    /**
     draw function with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset) = 0;

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
        uint32_t firstInstance, uint32_t instanceCount) = 0;
    
    /**
     设置纹理和采样器

     @param texture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index) = 0;
    
    /**
     设置立方体纹理和采样器

     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index) = 0;
    
    /**
     设置渲染纹理和采样器

     @param rendertexture 纹理句柄
     @param sampler 采样器句柄
     @param index 纹理通道索引
     */
    virtual void SetFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index) = 0;

    /**
     设置片元纹理和采样器

     @param resourceName 对应shader中的名字
     @param texture 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentTextureAndSampler(const std::string &resourceName, Texture2DPtr texture, TextureSamplerPtr sampler) = 0;

    /**
     设置片元立方体纹理和采样器

     @param resourceName 对应shader中的名字
     @param textureCube 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentTextureCubeAndSampler(const std::string& resourceName, TextureCubePtr textureCube, TextureSamplerPtr sampler) = 0;

    /**
     设置片元渲染纹理和采样器

     @param resourceName 对应shader中的名字
     @param rendertexture 纹理句柄
     @param sampler 采样器句柄
     */
    virtual void SetFragmentRenderTextureAndSampler(const std::string& resourceName, RenderTexturePtr renderTexture, TextureSamplerPtr sampler) = 0;
};

typedef std::shared_ptr<RenderEncoder> RenderEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_ENCODE_INCLUDE_H */
