//
//  VKComputePipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputePipeline.h"
#include "BaseLib/DebugBreaker.h"

NAMESPACE_RENDERCORE_BEGIN

VkDescriptorSetLayout CreateComputeDescriptorSetLayout(VkDevice device)
{
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

VKComputePipeline::VKComputePipeline(VulkanContextPtr context, const ShaderCode& shaderSource) : ComputePipeline(nullptr), mContext(context)
{
    // 这里spir-v的二进制还需要进行载入
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "CS";
    
    VkDescriptorSetLayout desSetLayout = CreateComputeDescriptorSetLayout(mContext->device);

    // 创建管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &desSetLayout;

    VkPipelineLayout computePipelineLayout= VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
    {
        printf("failed to create compute pipeline layout!");
        baselib::DebugBreak();
    }
    
    // 创建管线
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = computePipelineLayout;
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
    return;
}

NAMESPACE_RENDERCORE_END
