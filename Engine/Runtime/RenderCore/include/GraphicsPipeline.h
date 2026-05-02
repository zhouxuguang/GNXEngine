//
//  GraphicsPipeline.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#ifndef GNX_ENGINE_GRAPHIC_PIPELINE_INCLUDE_HHHFDF
#define GNX_ENGINE_GRAPHIC_PIPELINE_INCLUDE_HHHFDF

#include "RenderDescriptor.h"
#include "ShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class GraphicsPipeline
{
public:
    GraphicsPipeline(const GraphicsPipelineDesc& des);
    
    virtual ~GraphicsPipeline();
    
    virtual void AttachVertexShader(ShaderFunctionPtr shaderFunction) = 0;
    
    virtual void AttachFragmentShader(ShaderFunctionPtr shaderFunction) = 0;

    virtual void AttachGraphicsShader(GraphicsShaderPtr graphicsShader) = 0;
    
    // ===== Mesh Shader 接口（传统模式下调用为 no-op）=====
    
    /**
     * 绑定 Task Shader（可选，仅 Mesh Pipeline 使用）
     * Vulkan: Task Shader / Metal: Object Shader
     */
    virtual void AttachTaskShader(ShaderFunctionPtr shaderFunction) = 0;
    
    /**
     * 绑定 Mesh Shader（仅 Mesh Pipeline 使用）
     */
    virtual void AttachMeshShader(ShaderFunctionPtr shaderFunction) = 0;
    
    /**
     * 查询 pipeline 类型
     */
    PipelineType GetPipelineType() const { return mDesc.pipelineType; }
    
    const GraphicsPipelineDesc& GetDesc() const { return mDesc; }

    // 获取 Mesh Shader 的 threadgroup 大小（来自 SPIR-V ExecutionModeLocalSize / PSO 反射）
    virtual const uint32_t* GetMeshThreadgroupSize() const = 0;
    virtual const uint32_t* GetTaskThreadgroupSize() const = 0;

protected:
    GraphicsPipelineDesc mDesc;
};

typedef std::shared_ptr<GraphicsPipeline> GraphicsPipelinePtr;

//计算管线对象
class ComputePipeline
{
public:
    ComputePipeline(ShaderFunctionPtr kernelFunction)
    {
    }
    
    //获得计算着色器的线程组的大小
    virtual void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z){}
};

typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GRAPHIC_PIPELINE_INCLUDE_HHHFDF */
