//
//  VulkanDescriptorUtil.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/29.
//

#ifndef GNX_ENGINE_VULKAN_DESCRIPTOR_UTIL_INCLUDE_DJFSGHJ
#define GNX_ENGINE_VULKAN_DESCRIPTOR_UTIL_INCLUDE_DJFSGHJ

#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

class VulkanDescriptorUtil
{
public:
    static inline VkWriteDescriptorSet GetBufferWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorBufferInfo* bufferInfo,
        uint32_t descriptorCount = 1)
    {
        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pBufferInfo = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }
    
    static inline VkWriteDescriptorSet GetImageWriteDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorImageInfo *imageInfo,
        uint32_t descriptorCount = 1)
    {
        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }
    
    static inline VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
    {
        VkDescriptorImageInfo descriptorImageInfo = {};
        descriptorImageInfo.sampler = sampler;
        descriptorImageInfo.imageView = imageView;
        descriptorImageInfo.imageLayout = imageLayout;
        return descriptorImageInfo;
    }
    
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_DESCRIPTOR_UTIL_INCLUDE_DJFSGHJ */
