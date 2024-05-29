//
//  VKComputePipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputePipeline.h"
#include "VKShaderFunction.h"
#include "BaseLib/DebugBreaker.h"

NAMESPACE_RENDERCORE_BEGIN

#if 0
VkDescriptorSetLayout CreateComputeDescriptorSetLayout(VkDevice device)
{
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_com;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].pImmutableSamplers = nullptr;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = layoutBindings.data();
    
    VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) 
    {
        printf("failed to create compute descriptor set layout!\n");
    }
    
    return computeDescriptorSetLayout;
}
#endif

VKComputePipeline::VKComputePipeline(VulkanContextPtr context, const ShaderCode& shaderSource) : ComputePipeline(nullptr), mContext(context)
{
    // 这里spir-v的二进制还需要进行载入
    std::shared_ptr<VKShaderFunction> shaderFunction = std::make_shared<VKShaderFunction>(context);
    shaderFunction = shaderFunction->initWithShaderSourceInner(shaderSource, ShaderStage_Compute);
    assert(shaderFunction);
    
    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderFunction->GetShaderModule();
    computeShaderStageInfo.pName = shaderFunction->GetEntryName().c_str();
    
    // 根据shader反射出来的DescriptorSet信息创建各个DescriptorSetLayout
    const DescriptorSetLayoutDataVec& desSetLayouts = shaderFunction->GetDescriptorSets();
    std::vector<VkDescriptorSetLayout> desLayouts;
    desLayouts.resize(desSetLayouts.size());
    for (int i = 0; i < desSetLayouts.size(); i ++)
    {
        if (vkCreateDescriptorSetLayout(mContext->device, &desSetLayouts[i].create_info, nullptr, desLayouts.data() + i) != VK_SUCCESS)
        {
            printf("failed to create compute descriptor set layout!\n");
            baselib::DebugBreak();
        }
    }
    mDescriptorSetLayouts = std::move(desLayouts);

    // 创建管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)mDescriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = mDescriptorSetLayouts.data();

    if (vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        printf("failed to create compute pipeline layout!");
        baselib::DebugBreak();
    }
    
    // 删除VkDescriptorSetLayout
//    for (auto iter : desLayouts)
//    {
//        vkDestroyDescriptorSetLayout(mContext->device, iter, nullptr);
//    }
//    desLayouts.clear();
    
    // 创建管线
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.stage = computeShaderStageInfo;

    if (vkCreateComputePipelines(mContext->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS)
    {
        printf("failed to create compute pipeline!\n");
        baselib::DebugBreak();
    }
}

VKComputePipeline::~VKComputePipeline()
{
    //
}

void VKComputePipeline::GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z)
{
    x = 32;
    y = 1;
    z = 1;
}

NAMESPACE_RENDERCORE_END
