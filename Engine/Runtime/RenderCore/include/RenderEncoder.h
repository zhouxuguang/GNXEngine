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
     设置多边形填充模式（实心/线框），可在运行时动态切换
     */
    virtual void SetFillMode(FillMode fillMode) = 0;
    
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
    virtual void DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset, 
            uint32_t drawCount, uint32_t stride) = 0;

    /**
     * @brief 索引间接绘制（使用RCBuffer）
     * @param mode 图元类型
     * @param indexBuffer 索引缓冲区
     * @param indexBufferOffset 索引缓冲区偏移（索引个数，非字节数）
     * @param indirectBuffer 间接绘制参数buffer
     * @param indirectBufferOffset indirectBuffer偏移
     * @param drawCount 绘制次数
     * @param stride 步长
     */
    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
            int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
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

    /**
     设置mesh shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetMeshUniformBuffer(UniformBufferPtr buffer, int index) = 0;

    /**
     设置object (task) shader的uniformbuffer

     @param buffer buffer description
     @param index index description
     */
    virtual void SetTaskUniformBuffer(UniformBufferPtr buffer, int index) = 0;

    // ==================== 绘制接口 ====================
    
    /**
     draw function
     
     @param mode mode description
     @param offset offset description
     @param size size description
     */
    virtual void DrawPrimitives(PrimitiveMode mode, int offset, int size) = 0;

    /**
     draw function with instance
     
     @param mode mode description
     @param offset offset description
     @param size size description
     @param firstInstance 第一个实例的索引
     @param instanceCount 实例的个数
     */
    virtual void DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount) = 0;
    
    /**
     draw function with index
     
     @param mode mode description
     @param size size description
     @param buffer buffer description
     @param offset offset description
     @param baseVertex 顶点偏移量，默认为0
     */
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex = 0) = 0;

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

    /**
     * @brief 设置顶点着色器的纹理和采样器
     *
     * @param resourceName 对应shader中的名字
     * @param texture 纹理句柄
     * @param sampler 采样器句柄
     */
    virtual void SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) = 0;

    /**
     * @brief 设置Mesh Shader的纹理和采样器
     *
     * @param resourceName 对应shader中的名字
     * @param texture 纹理句柄
     * @param sampler 采样器句柄
     */
    virtual void SetMeshTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) = 0;

    /**
     * @brief 设置Task (Object) Shader的纹理和采样器
     *
     * @param resourceName 对应shader中的名字
     * @param texture 纹理句柄
     * @param sampler 采样器句柄
     */
    virtual void SetTaskTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) = 0;

    // ==================== Mesh Shader 绘制接口 ====================
    
    /**
     * @brief 绘制 Mesh Tasks
     * @param groupCountX mesh task group 数量 X
     * @param groupCountY mesh task group 数量 Y
     * @param groupCountZ mesh task group 数量 Z
     */
    virtual void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
    
    /**
     * @brief 间接绘制 Mesh Tasks
     * @param buffer 包含 DrawMeshTasksIndirectCommand 数组的 buffer
     * @param offset buffer 偏移量（字节）
     * @param drawCount 绘制次数
     * @param stride 每次绘制的步长（字节）
     */
    virtual void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                       uint32_t drawCount, uint32_t stride) = 0;

    // ==================== 动态渲染状态接口 ====================

    /**
     * @brief 设置裁剪矩形
     * @param x 裁剪区域左上角 x
     * @param y 裁剪区域左上角 y
     * @param width 裁剪区域宽度
     * @param height 裁剪区域高度
     */
    virtual void SetScissorRect(int x, int y, uint32_t width, uint32_t height) = 0;

    /**
     * @brief 设置深度偏移
     * @param bias 深度偏移常量因子
     * @param slopeScale 深度偏移斜率因子
     * @param clamp 深度偏移钳制值
     */
    virtual void SetDepthBias(float bias, float slopeScale, float clamp) = 0;

    /**
     * @brief 设置模板参考值
     * @param frontRef 前面模板参考值
     * @param backRef 背面模板参考值
     */
    virtual void SetStencilReference(uint32_t frontRef, uint32_t backRef) = 0;
};

typedef std::shared_ptr<RenderEncoder> RenderEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_ENCODE_INCLUDE_H */
