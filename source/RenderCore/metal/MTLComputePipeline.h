//
//  MTLComputePipeline.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_MTL_COMPUTE_PIPELINE_INCLUDE_H
#define GNX_ENGINE_MTL_COMPUTE_PIPELINE_INCLUDE_H

#include "MTLRenderDefine.h"
#include "GraphicsPipeline.h"
#include "MTLDeviceExtension.h"
#include "MTLShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLComputePipeline : public ComputePipeline
{
public:
    MTLComputePipeline(id<MTLDevice> device, ShaderFunctionPtr kernelFunction) : ComputePipeline(kernelFunction)
    {
        @autoreleasepool 
        {
            //assert(kernelFunction);
            NSError* error = nil;
            MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(kernelFunction);
            mComputePSO = [device newComputePipelineStateWithFunction: shaderPtr->getShaderFunction() error:&error];
        }
    }
    
    //获得计算着色器的线程组的大小
    virtual void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z)
    {
        @autoreleasepool
        {
            // Calculate the maximum threads per threadgroup based on the thread execution width.
            NSUInteger w = mComputePSO.threadExecutionWidth;
            NSUInteger h = mComputePSO.maxTotalThreadsPerThreadgroup / w;
            
            x = (uint32_t)w;
            y = (uint32_t)h;
            z = 1;
        }
    }
    
    id<MTLComputePipelineState> GetMTLComputePipelineState() const
    {
        return mComputePSO;
    }
    
private:
    id<MTLComputePipelineState> mComputePSO;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMPUTE_PIPELINE_INCLUDE_H */
