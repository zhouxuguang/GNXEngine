//
//  VKComputePipeline.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH
#define GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH

#include "VulkanContext.h"
#include "GraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputePipeline : public ComputePipeline
{
public:
    VKComputePipeline(VulkanContextPtr context, const char* pszShaderString);
    
    ~VKComputePipeline();
    
    //获得计算着色器的线程组的大小
    virtual void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z);
    
private:
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH */
