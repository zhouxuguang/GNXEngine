//
//  VKComputeEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputeEncoder.h"
#include "VKComputePipeline.h"
#include "VKComputeBuffer.h"
#include "VKTexture2D.h"
#include "VKRenderTexture.h"
#include "VKComputePipeline.h"
#include "VulkanDescriptorUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKComputeEncoder::VKComputeEncoder(VulkanContextPtr context, VkCommandBuffer commandBuffer)
{
    mContext = context;
    mCommandBuffer = commandBuffer;
}

VKComputeEncoder::~VKComputeEncoder()
{
}

void VKComputeEncoder::SetComputePipeline(ComputePipelinePtr computePipeline)
{
    if (!computePipeline)
    {
        return;
    }
    VKComputePipeline *vkPipeline = (VKComputePipeline*)computePipeline.get();
    mVKPipeline = vkPipeline;
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetPipeline());
}

#if 0
// Provided by VK_KHR_push_descriptor
void vkCmdPushDescriptorSetKHR(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    set,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites);
#endif

void VKComputeEncoder::SetBuffer(ComputeBufferPtr buffer, uint32_t index)
{
    if (!buffer)
    {
        return;
    }
    VKComputeBuffer *vkComputeBuffer = (VKComputeBuffer*)buffer.get();
//    typedef VkResult (VKAPI_PTR *PFN_vkFreeDescriptorSets)(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
    
    VkDescriptorSet descriptorSet = AllocDescriptorSet(mContext->device, mContext->computeDescriptorPool, 
                                                       mVKPipeline->GetDescriptorSetLayouts()[0]);
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfo.buffer = vkComputeBuffer->GetBuffer();
    
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(descriptorSet,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, &bufferInfo);
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), 0, 1, &writeDescriptorSet);
    
    vkFreeDescriptorSets(mContext->device, mContext->computeDescriptorPool, 1, &descriptorSet);
}

void VKComputeEncoder::SetTexture(Texture2DPtr texture, uint32_t index)
{
    //
}

void VKComputeEncoder::SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    //
}

void VKComputeEncoder::Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
{
    vkCmdDispatch(mCommandBuffer, threadGroupsX, threadGroupsY, threadGroupsZ);
}

void VKComputeEncoder::EndEncode()
{
    vkCmdEndRenderingKHR(mCommandBuffer);
}

NAMESPACE_RENDERCORE_END
