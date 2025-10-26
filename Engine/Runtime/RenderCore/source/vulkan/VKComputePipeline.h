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
#include "VKShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputePipeline : public ComputePipeline
{
public:
    VKComputePipeline(VulkanContextPtr context, const ShaderCode& shaderSource);
    
    ~VKComputePipeline();
    
    //获得计算着色器的线程组的大小
    virtual void GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z);
    
    VkPipeline GetPipeline() const
    {
        return mPipeline;
    }
    
    const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const
    {
        return mDescriptorSetLayouts;
    }
    
    VkPipelineLayout GetPipelineLayout() const
    {
        return mPipelineLayout;
    }
    
    uint32_t GetSetOffset(DescriptorType descriptorType) const
    {
        return mStageSetOffsets[descriptorType];
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkPipeline mPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
    WorkGroupSize mWorkGroupSize;
    
    uint32_t mStageSetOffsets[DESCRIPTOR_TYPE_MAX];   //每一种资源所在的set的索引
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_COMPUTE_PIPELINE_INCLUDE_SDKFGJDSH */
