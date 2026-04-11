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
#include "MTLShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLComputePipeline : public ComputePipeline
{
public:
    MTLComputePipeline(id<MTLDevice> device, ShaderFunctionPtr kernelFunction);
    
    ~MTLComputePipeline()
    {
    }
    
    //获得计算着色器的线程组的大小
    virtual void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z);
    
    id<MTLComputePipelineState> GetMTLComputePipelineState() const
    {
        return mComputePSO;
    }
    
    /**
     * @brief 通过资源名获取绑定索引（Metal内部使用）
     */
    NSUInteger GetResourceIndex(const std::string& resourceName) const
    {
        auto iter = mResourceMap.find(resourceName);
        if (iter != mResourceMap.end())
        {
            return iter->second;
        }
        
        return InvalidBindingIndex;
    }
    

private:
    id<MTLComputePipelineState> mComputePSO;
    std::unordered_map<std::string, NSUInteger> mResourceMap;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMPUTE_PIPELINE_INCLUDE_H */
