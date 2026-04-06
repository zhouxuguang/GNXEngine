//
//  VKComputeEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputeEncoder.h"
#include "VKComputePipeline.h"
#include "VulkanDescriptorUtil.h"
#include "VKTextureBase.h"
#include "VKUniformBuffer.h"
#include "VulkanBufferUtil.h"

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

void VKComputeEncoder::SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    if (!mVKPipeline)
    {
        assert(false);
        return;
    }

    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    // bind资源
    std::shared_ptr<VKUniformBuffer> vkBuffer = std::dynamic_pointer_cast<VKUniformBuffer>(buffer);

	VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vkBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

	// 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
	VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bindIndex, &bufferInfo);
	writeDescriptorSet.dstSet = 0;   //这句也是可以的

	uint32_t bufSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), bufSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetStorageBuffer(RCBufferPtr buffer, uint32_t index)
{
    if (!buffer || !mVKPipeline)
    {
        return;
    }
    
    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (!vkBuffer)
    {
        return;
    }
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vkBuffer->GetVkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index, &bufferInfo);
    writeDescriptorSet.dstSet = 0;
    
    uint32_t bufSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_STORAGE_BUFFER);
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), bufSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer)
{
    if (!mVKPipeline || !buffer)
    {
        return;
    }
    
    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    SetStorageBuffer(buffer, bindIndex);
}

void VKComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKTextureBasePtr vkTexture2D = std::dynamic_pointer_cast<VKTextureBase>(texture);

    // 使用纹理的当前layout
    VkImageLayout imageLayout = vkTexture2D->GetCurrentLayout();

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = vkTexture2D->GetImageView()->GetHandle();
    imageInfo.imageLayout = imageLayout;
    imageInfo.sampler = nullptr;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                                    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    uint32_t imageSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), imageSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKTextureBasePtr vkRenderTex = std::dynamic_pointer_cast<VKTextureBase>(texture);

    // Sampled image descriptor可以使用具体的layout，使用纹理的当前layout
    VkImageLayout imageLayout = vkRenderTex->GetCurrentLayout();

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = vkRenderTex->GetImageView()->GetHandle();
    imageInfo.imageLayout = imageLayout;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    uint32_t imageSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), imageSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture)
{
    if (!mVKPipeline || !texture)
    {
        return;
    }
    
    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    SetTexture(texture, bindIndex);
}

void VKComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel)
{
    if (!mVKPipeline || !texture)
    {
        return;
    }
    
    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    SetTexture(texture, mipLevel, bindIndex);
}

void VKComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKTextureBasePtr vkRenderTex = std::dynamic_pointer_cast<VKTextureBase>(texture);

    // Sampled image descriptor可以使用具体的layout，使用纹理的当前layout
    VkImageLayout imageLayout = vkRenderTex->GetCurrentLayout();

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = vkRenderTex->GetImageView()->GetHandle();
    imageInfo.imageLayout = imageLayout;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    uint32_t imageSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_STORAGE_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), imageSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    if (!texture)
    {
        return;
    }
    VKTextureBasePtr vkRenderTex = std::dynamic_pointer_cast<VKTextureBase>(texture);

    // Sampled image descriptor可以使用具体的layout，使用纹理的当前layout
    VkImageLayout imageLayout = vkRenderTex->GetCurrentLayout();

    VkDescriptorImageInfo imageInfo = {};
    // 使用特定 mip level 的 ImageView
    imageInfo.imageView = vkRenderTex->GetMipLevelImageView(mipLevel)->GetHandle();
    imageInfo.imageLayout = imageLayout;
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE,
                                                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, &imageInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    uint32_t imageSetOffset = mVKPipeline->GetSetOffset(DESCRIPTOR_TYPE_STORAGE_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mVKPipeline->GetPipelineLayout(), imageSetOffset, 1, &writeDescriptorSet);
}

void VKComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture)
{
    if (!mVKPipeline || !texture)
    {
        return;
    }
    
    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    SetOutTexture(texture, bindIndex);
}

void VKComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel)
{
    if (!mVKPipeline || !texture)
    {
        return;
    }
    
    uint32_t bindIndex = mVKPipeline->GetResourceBindIndex(resourceName);
    if (-1 == bindIndex)
    {
        assert(false);
        return;
    }
    
    SetOutTexture(texture, mipLevel, bindIndex);
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
