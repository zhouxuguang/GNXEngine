//
//  VulkanFeatures.h
//  rendercore
//
//  Created by zhouxuguang on 2026/5/28.
//

#ifndef GNX_ENGINE_VK_FEATURES_FSDJDDFJ_INCLUDE
#define GNX_ENGINE_VK_FEATURES_FSDJDDFJ_INCLUDE

#include "VKRenderDefine.h"
#include "VKUtil.h"

NAMESPACE_RENDERCORE_BEGIN

struct DeviceExtFeature 
{
    VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDevicePortabilitySubsetFeaturesKHR portabilitySubsetFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR };
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT};
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT};
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};

    void Init(VkPhysicalDevice physicalDevice)
    {
        AddToPNextChain(&features2, &features11);
        AddToPNextChain(&features2, &features12);
        AddToPNextChain(&features2, &features13);
        AddToPNextChain(&features2, &portabilitySubsetFeature);
        AddToPNextChain(&features2, &meshShaderFeatures);
        AddToPNextChain(&features2, &dynamicRenderingFeatures);
        AddToPNextChain(&features2, &timelineSemaphoreFeatures);
        AddToPNextChain(&features2, &extendedDynamicStateFeaturesEXT);
        AddToPNextChain(&features2, &extendedDynamicState2FeaturesEXT);
        AddToPNextChain(&features2, &extendedDynamicState3FeaturesEXT);
        AddToPNextChain(&features2, &descriptorIndexingFeatures);

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    }
};

struct DeviceExtProperties
{
    VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceVulkan11Properties features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};

    void Init(VkPhysicalDevice physicalDevice)
    {
        AddToPNextChain(&properties2, &features11);
        AddToPNextChain(&properties2, &features12);
        AddToPNextChain(&properties2, &features13);
        AddToPNextChain(&properties2, &meshShaderProperties);

        vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
    }
};

NAMESPACE_RENDERCORE_END

#endif