//
//  VulkanExtension.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/31.
//

#include "VulkanExtension.h"

NAMESPACE_RENDERCORE_BEGIN

void VulkanExtension::Init(VkPhysicalDevice physicalDevice)
{
    InitExtendedDynamicState(physicalDevice);
}

void VulkanExtension::InitExtendedDynamicState(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT = {};
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT = {};
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT = {};
    
    // 获得完整的扩展动态状态
    extendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeaturesEXT.pNext = &extendedDynamicState2FeaturesEXT;
    extendedDynamicState2FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    extendedDynamicState2FeaturesEXT.pNext = &extendedDynamicState3FeaturesEXT;
    extendedDynamicState3FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    extendedDynamicState3FeaturesEXT.pNext = nullptr;

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.pNext = &extendedDynamicStateFeaturesEXT;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures2);
    
    // Check what dynamic states are supported by the current implementation
    // Checking for available features is probably sufficient, but retained redundant extension checks for clarity and consistency
    enabledExtendedDynamicState = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) && extendedDynamicStateFeaturesEXT.extendedDynamicState;
    enabledExtendedDynamicState2 = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) && extendedDynamicState2FeaturesEXT.extendedDynamicState2;
    enabledExtendedDynamicState3 = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) && extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable && extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation;
}

bool VulkanExtension::ExtensionSupported(const char* name)
{
    uint32_t count = (uint32_t)mDeviceExtensions.size();
    for (uint32_t k = 0; k < count; ++k)
    {
        if (!strcmp(mDeviceExtensions[k].extensionName, name))
        {
            return true;
        }
    }
    
    return false;
}

NAMESPACE_RENDERCORE_END
