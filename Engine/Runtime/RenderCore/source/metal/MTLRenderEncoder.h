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
    
    void EndEncode() override;
    
    /**
     设置图形管线
     */
    void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline) override;

    /**
     设置多边形填充模式（实心/线框）
     */
    void SetFillMode(FillMode fillMode) override;
    
    /**
     Description
     
     @param buffer buffer对象
     @param index 绑定的索引
     */
    void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index) override;
    
    // RCBuffer接口
    void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index) override;
    
    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage) override;
    
    void DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride) override;

    void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
        int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
        uint32_t drawCount, uint32_t stride) override;

    void DrawIndexedPrimitivesIndirectCount(PrimitiveMode mode, IndexBufferPtr indexBuffer,
        int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
        RCBufferPtr countBuffer, uint32_t countBufferOffset,
        uint32_t maxDrawCount, uint32_t stride) override;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    void SetVertexUniformBuffer(UniformBufferPtr buffer, int index) override;
    
    /**
     设置uniformbuffer的索引
     
     @param buffer buffer description
     @param index index description
     */
    void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index) override;
    
    void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture) override;
    
    /**
     设置顶点uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    /**
     设置片元uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    /**
     设置mesh shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    void SetMeshUniformBuffer(UniformBufferPtr buffer, int index) override;

    /**
     设置task shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    void SetTaskUniformBuffer(UniformBufferPtr buffer, int index) override;

    /**
     设置mesh shader的uniformbuffer（按资源名绑定）

     @param resourceName shader中的资源名
     @param buffer buffer description
     */
    void SetMeshUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    /**
     设置task shader的uniformbuffer（按资源名绑定）

     @param resourceName shader中的资源名
     @param buffer buffer description
     */
    void SetTaskUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    void DrawPrimitives(PrimitiveMode mode, int offset, int size) override;
    
    void DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount) override;
    
    /**
     draw funton with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     */
    void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex = 0) override;
    
    /**
     draw function with index instance
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset 索引的偏移
     @param offset firstInstance 第一个实例的索引
     @param offset instanceCount 实例的个数
     */
    void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
                                               uint32_t firstInstance, uint32_t instanceCount) override;
    
    void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetMeshTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetTaskTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;

    // ===== Mesh Shader 绘制接口 =====
    void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                       uint32_t drawCount, uint32_t stride) override;

    // ===== 动态渲染状态接口 =====
    void SetScissorRect(int x, int y, uint32_t width, uint32_t height) override;
    void SetDepthBias(float bias, float slopeScale, float clamp) override;
    void SetStencilReference(uint32_t frontRef, uint32_t backRef) override;

private:
    id <MTLRenderCommandEncoder> mRenderEncoder = nil;
    
    FrameBufferFormat mFrameBufferFormat;
    MTLGraphicsPipelinePtr mMtlGraphicsPipeline = nil;

    // Metal Fence 回调：EndEncode 时由 CommandBuffer 设置，用于冲刷 fence 更新
    mutable std::function<void(id<MTLRenderCommandEncoder>)> mFenceCallback = nullptr;

public:
    void SetFenceCallback(std::function<void(id<MTLRenderCommandEncoder>)> cb) const
    {
        mFenceCallback = std::move(cb);
    }
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_ENCODER_INCLUDE_H */
