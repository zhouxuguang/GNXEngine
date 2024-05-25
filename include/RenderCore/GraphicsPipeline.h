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
    GraphicsPipeline(const GraphicsPipelineDescriptor& des);
    
    virtual ~GraphicsPipeline();
    
    virtual void attachVertexShader(ShaderFunctionPtr shaderFunction) = 0;
    
    virtual void attachFragmentShader(ShaderFunctionPtr shaderFunction) = 0;
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
