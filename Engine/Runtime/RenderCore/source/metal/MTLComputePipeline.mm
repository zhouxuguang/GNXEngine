//
//  MTLComputePipeline.mm
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "MTLComputePipeline.h"

NAMESPACE_RENDERCORE_BEGIN

MTLComputePipeline::MTLComputePipeline(id<MTLDevice> device, ShaderFunctionPtr kernelFunction) : ComputePipeline(kernelFunction)
{
    @autoreleasepool
    {
        assert(kernelFunction);
        NSError* error = nil;
        MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(kernelFunction);
        
        MTLComputePipelineReflection* reflectionObj = nil;
        MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
        mComputePSO = [device newComputePipelineStateWithFunction: shaderPtr->GetShaderFunction() 
                                                     options: option reflection:&reflectionObj error:&error];
        
        if (!reflectionObj)
        {
            return;
        }
        
    #if SUPPORTED_NEW_REFLECT
        for (id<MTLBinding> arg in reflectionObj.bindings)
        {
            NSLog(@"MTLComputePipeline::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
            
            mResourceMap[arg.name.UTF8String] = arg.index;
        }
    #else
        for (MTLArgument * arg in reflectionObj.arguments)
        {
            NSLog(@"MTLComputePipeline::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
            
            mResourceMap[arg.name.UTF8String] = arg.index;
        }
    #endif
    }
}

void MTLComputePipeline::GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z)
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

NAMESPACE_RENDERCORE_END
