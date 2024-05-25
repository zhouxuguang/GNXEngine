//
//  VulkanDeviceUtil.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_RENDER_CORE_VULKAN_DEVICE_UTIL_INCLUDE_JHDSGHH
#define GNX_ENGINE_RENDER_CORE_VULKAN_DEVICE_UTIL_INCLUDE_JHDSGHH

#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

//vulkan 物理设备的辅助函数

class VulkanDeviceUtil
{
public:
    static bool IsDeviceSuitable(VkPhysicalDevice device);

    //获得gpu的个数
    static void GetPhysicalDevices(VkInstance pInstance, std::vector<VkPhysicalDevice>& gpuDevices);

    //查找内存对应的索引
    static bool MapMemoryTypeToIndex(VkPhysicalDevice gpuDevice, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);

    //获得最大可用的采样数目
    static VkSampleCountFlagBits GetMaxUsableSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties);
    
    //查找合适的深度模板缓冲格式
    static VkFormat FindDepthStencilFormat(const VulkanContext& vulkanContext);
};


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_CORE_VULKAN_DEVICE_UTIL_INCLUDE_JHDSGHH */
