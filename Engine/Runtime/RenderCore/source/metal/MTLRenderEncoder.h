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
#include "MTLRCBuffer.h"

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
    
    // 新的RCBuffer接口
    virtual void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index);
    
    virtual void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage);
    
    virtual void DrawPrimitvesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);

    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetVertexUniformBuffer(UniformBufferPtr buffer, int index);

    virtual void SetVertexUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer);
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index);
    
    virtual void SetFragmentUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer);
    
    virtual void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture);
    
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

    virtual void DrawPrimitvesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);

    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);
    
    virtual void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler);

private:
    id <MTLRenderCommandEncoder> mRenderEncoder = nil;
    
    FrameBufferFormat mFrameBufferFormat;
    MTLGraphicsPipelinePtr mMtlGraphicsPipeline = nil;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H */
