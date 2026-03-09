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
#include "TextureSampler.h"
#include "RCTexture.h"
#include "RCBuffer.h"

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
    
    // ==================== RCBuffer接口 ====================
    
    /**
     * @brief 设置RCBuffer作为顶点缓冲区
     * @param buffer RCBuffer指针（需要包含VertexBuffer用途）
     * @param offset 偏移量
     * @param index 绑定索引
     */
    virtual void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index) = 0;
    
    /**
     * @brief 设置RCBuffer作为SSBO
     * @param resourceName 资源名称
     * @param buffer RCBuffer指针（需要包含StorageBuffer用途）
     * @param stage shader阶段
     */
    virtual void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage) = 0;
    
    /**
     * @brief 间接绘制（使用RCBuffer）
     * @param mode 图元类型
     * @param buffer 间接绘制参数buffer（需要包含IndirectBuffer用途）
     * @param offset buffer偏移
     * @param drawCount 绘制次数
     * @param stride 步长
     */
    virtual void DrawPrimitvesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset, 
            uint32_t drawCount, uint32_t stride) = 0;

    /**
     * @brief 索引间接绘制（使用RCBuffer）
     */
    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
		    uint32_t drawCount, uint32_t stride) = 0;
    
    // ==================== UniformBuffer接口 ====================
    
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
     设置片元存储图像

     @param buffer buffer description
     @param index index description
     */
    virtual void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture) = 0;

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
    
    // ==================== 绘制接口 ====================
    
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
     @param firstInstance 第一个实例的索引
     @param instanceCount 实例的个数
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
     @param buffer 所以你buffer
     @param offset 索引的偏移
     @param firstInstance 第一个实例的索引
     @param instanceCount 实例的个数
     */
    virtual void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, 
        uint32_t firstInstance, uint32_t instanceCount) = 0;

    /**
     * @brief 设置片源的纹理和采样器
     * 
     * @param resourceName 对应shader中的名字
     * @param texture 纹理句柄
     * @param sampler 采样器句柄
     */
    virtual void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) = 0;
};

typedef std::shared_ptr<RenderEncoder> RenderEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_ENCODE_INCLUDE_H */
