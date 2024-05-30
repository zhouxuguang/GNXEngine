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
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfo.buffer = vkComputeBuffer->GetBuffer();
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, &bufferInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), 0, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetTexture(Texture2DPtr texture, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKTexture2D *vkTexture2D = (VKTexture2D*)texture.get();
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = vkTexture2D->getVKImageView()->GetHandle();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), 1, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKRenderTexture *vkRenderTex = (VKRenderTexture*)texture.get();
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = vkRenderTex->GetImageView()->GetHandle();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), 1, 1, &writeDescriptorSet);
}

void VKComputeEncoder::Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
{
    vkCmdDispatch(mCommandBuffer, threadGroupsX, threadGroupsY, threadGroupsZ);
}

void VKComputeEncoder::EndEncode()
{
    // vulkan中的计算着色器没有pass的概念
    //vkCmdEndRenderingKHR(mCommandBuffer);
}

NAMESPACE_RENDERCORE_END
