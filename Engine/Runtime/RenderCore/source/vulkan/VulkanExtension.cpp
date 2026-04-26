//
//  VulkanExtension.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/31.
//

#include "VulkanExtension.h"

NAMESPACE_RENDERCORE_BEGIN

void VulkanExtension::Init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties)
{
    mPhysicalDeviceProperties = physicalDeviceProperties;
	InitExtendedDynamicState(physicalDevice);

	enabledDynamicRendering = ExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    enablePortabilitySubset = ExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    enablePushDesDescriptor = ExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    enableDescriptorUpdateTemplate = ExtensionSupported(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME);

    // host image copy关联的扩展
    enableFormatFeatureFlags2 = ExtensionSupported(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
    enableCopyCommands2 = ExtensionSupported(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
    enableHostImageCopy = ExtensionSupported(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME) && 
        enableFormatFeatureFlags2 && enableCopyCommands2/* && mPhysicalDeviceProperties.deviceID != 0x1C81*/;
    
    enableDeviceFault = ExtensionSupported(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    
    // Mesh Shader 的依赖扩展检测
    enableShaderFloatControls = ExtensionSupported(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    enableSpirv14 = ExtensionSupported(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

    // Mesh Shader 扩展检测（仅使用标准的 EXT 扩展，与 Metal 等其他图形 API 通用）
    // 需要 VK_KHR_spirv_1_4 和 VK_KHR_shader_float_controls 作为依赖
    enableMeshShaderEXT = ExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME) &&
                          enableSpirv14 && enableShaderFloatControls;

    // 查询 Mesh Shader 实际 feature 支持情况
    if (enableMeshShaderEXT)
    {
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.pNext = nullptr;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &meshShaderFeatures;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        meshShaderSupported = meshShaderFeatures.meshShader == VK_TRUE;
        taskShaderSupported = meshShaderFeatures.taskShader == VK_TRUE;

        // 如果 meshShader feature 不支持，则整体禁用 mesh shader
        if (!meshShaderSupported)
        {
            enableMeshShaderEXT = false;
        }
    }
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
    enabledExtendedDynamicState = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) && 
        extendedDynamicStateFeaturesEXT.extendedDynamicState;
    enabledExtendedDynamicState2 = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) && 
        extendedDynamicState2FeaturesEXT.extendedDynamicState2;
    enabledExtendedDynamicState3 = ExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) && 
        extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable && 
        extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation;
}

bool VulkanExtension::IsExtensionSupported(const char* name) const
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
