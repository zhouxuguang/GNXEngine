//
//  VulkanDeviceUtil.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanDeviceUtil.h"

NAMESPACE_RENDERCORE_BEGIN

bool VulkanDeviceUtil::IsDeviceSuitable(VkPhysicalDevice device)
{
    if (!device)
    {
        return false;
    }
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
}

void VulkanDeviceUtil::GetPhysicalDevices(VkInstance pInstance, std::vector<VkPhysicalDevice> &gpuDevices)
{
    if (VK_NULL_HANDLE == pInstance)
    {
        return;
    }

    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(pInstance, &gpu_count, NULL);
    gpuDevices.resize(gpu_count);
    vkEnumeratePhysicalDevices(pInstance, &gpu_count, gpuDevices.data());
}

bool VulkanDeviceUtil::MapMemoryTypeToIndex(VkPhysicalDevice gpuDevice,
                              uint32_t typeBits,
                              VkFlags requirements_mask,
                              uint32_t *typeIndex)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(gpuDevice, &memoryProperties);

    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    *typeIndex = 0;
    return true;
}

static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
                                    VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    
    return candidates[0];
}

VkSampleCountFlagBits VulkanDeviceUtil::GetMaxUsableSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties)
{
#ifdef ENABLE_VULKAN_MSAA
    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts,
            physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
#endif

    return VK_SAMPLE_COUNT_1_BIT;
}

VkFormat VulkanDeviceUtil::FindDepthStencilFormat(const VulkanContext& vulkanContext)
{
    std::vector<VkFormat> depthStencilFormats;
    depthStencilFormats.reserve(3);

    depthStencilFormats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
    depthStencilFormats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);

    return FindSupportedFormat(vulkanContext.physicalDevice,
                               depthStencilFormats,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

NAMESPACE_RENDERCORE_END
