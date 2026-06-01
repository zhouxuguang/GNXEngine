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
    VkPhysicalDeviceHostImageCopyFeaturesEXT hostImageCopyFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT};
	
#ifdef ENABLE_NSIGHT_AFTERMATH
    VkDeviceDiagnosticsConfigCreateInfoNV diagnosticsConfigCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV, 0, 
	    VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
		VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
		VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
		VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV };
#endif

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
        AddToPNextChain(&features2, &hostImageCopyFeatures);
#ifdef ENABLE_NSIGHT_AFTERMATH
        AddToPNextChain(&features2, &diagnosticsConfigCreateInfo);
#endif

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        features11.pNext = nullptr;
        features12.pNext = nullptr;
        features13.pNext = nullptr;
        portabilitySubsetFeature.pNext = nullptr;
        meshShaderFeatures.pNext = nullptr;
        dynamicRenderingFeatures.pNext = nullptr;
        timelineSemaphoreFeatures.pNext = nullptr;
        extendedDynamicStateFeaturesEXT.pNext = nullptr;
        extendedDynamicState2FeaturesEXT.pNext = nullptr;
        extendedDynamicState3FeaturesEXT.pNext = nullptr;
        descriptorIndexingFeatures.pNext = nullptr;
        hostImageCopyFeatures.pNext = nullptr;
#ifdef ENABLE_NSIGHT_AFTERMATH
        diagnosticsConfigCreateInfo.pNext = nullptr;
#endif
    }
};

struct DeviceExtProperties
{
    VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceVulkan11Properties features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};

	VkPhysicalDeviceHostImageCopyPropertiesEXT hostImageCopyProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES_EXT};
	std::vector<VkImageLayout> hostImageCopySrcLayoutsStorage;
	std::vector<VkImageLayout> hostImageCopyDstLayoutsStorage;

    VkPhysicalDeviceSubgroupProperties subgroupProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };

    void Init(VkPhysicalDevice physicalDevice)
    {
        AddToPNextChain(&properties2, &features11);
        AddToPNextChain(&properties2, &features12);
        AddToPNextChain(&properties2, &features13);
        AddToPNextChain(&properties2, &meshShaderProperties);

		constexpr uint32_t kMaxLayoutCount = 200;
		hostImageCopySrcLayoutsStorage.resize(kMaxLayoutCount, VK_IMAGE_LAYOUT_UNDEFINED);
		hostImageCopyDstLayoutsStorage.resize(kMaxLayoutCount, VK_IMAGE_LAYOUT_UNDEFINED);
		hostImageCopyProperties.copySrcLayoutCount = kMaxLayoutCount;
		hostImageCopyProperties.copyDstLayoutCount = kMaxLayoutCount;
		hostImageCopyProperties.pCopySrcLayouts = hostImageCopySrcLayoutsStorage.data();
		hostImageCopyProperties.pCopyDstLayouts = hostImageCopyDstLayoutsStorage.data();
        AddToPNextChain(&properties2, &hostImageCopyProperties);

        AddToPNextChain(&properties2, &subgroupProperties);

        vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

        properties2.pNext = nullptr;
        features11.pNext = nullptr;
        features12.pNext = nullptr;
        features13.pNext = nullptr;
        meshShaderProperties.pNext = nullptr;
        subgroupProperties.pNext = nullptr;
    }
};

NAMESPACE_RENDERCORE_END

#endif