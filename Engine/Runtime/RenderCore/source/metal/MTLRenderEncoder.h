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
     设置多边形填充模式（实心/线框）
     */
    virtual void SetFillMode(FillMode fillMode);
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    virtual void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index);
    
    // RCBuffer接口
    virtual void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index);
    
    virtual void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage);
    
    virtual void DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);

    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
        int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
        uint32_t drawCount, uint32_t stride);
    
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
     设置mesh shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetMeshUniformBuffer(UniformBufferPtr buffer, int index);

    /**
     设置task shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetTaskUniformBuffer(UniformBufferPtr buffer, int index);

    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void DrawPrimitives(PrimitiveMode mode, int offset, int size);
    
    virtual void DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount);
    
    /**
     draw funton with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex = 0);
    
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
    
    virtual void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler);
    virtual void SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler);

    // ===== Mesh Shader 绘制接口 =====
    virtual void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    virtual void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                       uint32_t drawCount, uint32_t stride);

    // ===== 动态渲染状态接口 =====
    virtual void SetScissorRect(int x, int y, uint32_t width, uint32_t height);
    virtual void SetDepthBias(float bias, float slopeScale, float clamp);
    virtual void SetStencilReference(uint32_t frontRef, uint32_t backRef);

private:
    id <MTLRenderCommandEncoder> mRenderEncoder = nil;
    
    FrameBufferFormat mFrameBufferFormat;
    MTLGraphicsPipelinePtr mMtlGraphicsPipeline = nil;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H */
