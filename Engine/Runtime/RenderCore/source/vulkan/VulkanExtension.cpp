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

	enabledDynamicRendering = IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    enablePortabilitySubset = IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    enablePushDesDescriptor = IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    enableDescriptorUpdateTemplate = IsExtensionSupported(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME);

    // host image copy关联的扩展
    enableFormatFeatureFlags2 = IsExtensionSupported(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
    enableCopyCommands2 = IsExtensionSupported(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
    enableHostImageCopy = IsExtensionSupported(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME) &&
        enableFormatFeatureFlags2 && enableCopyCommands2;

    // NVIDIA 驱动的 VK_EXT_host_image_copy 实现与 RenderDoc Layer 存在兼容性问题，
    // 当 RenderDoc Layer 激活时可能导致 vkCreateDevice 崩溃或数据拷贝错误。
    // 检测 RenderDoc Layer，仅在存在时禁用此扩展。
    if (enableHostImageCopy)
    {
        uint32_t instanceLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

        bool hasRenderDocLayer = false;
        for (const auto& layer : instanceLayers)
        {
            if (strstr(layer.layerName, "RenderDoc") != nullptr)
            {
                hasRenderDocLayer = true;
                break;
            }
        }

        if (hasRenderDocLayer)
        {
            enableHostImageCopy = false;
        }
    }
    
    // Subgroup 操作检测（Wave* 系列 HLSL 指令）
    // 保守策略：所有 core subgroup 操作都必须支持，waveIntrinsics 才返回 true
    // Core 操作: BASIC | VOTE | ARITHMETIC | BALLOT | SHUFFLE | SHUFFLE_RELATIVE | CLUSTERED | QUAD = 0xFF
    {
        VkPhysicalDeviceSubgroupProperties subgroupProps = {};
        subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        subgroupProps.pNext = nullptr;

        VkPhysicalDeviceProperties2 props2 = {};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props2.pNext = &subgroupProps;
        vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

        static const VkSubgroupFeatureFlags REQUIRED_SUBGROUP_OPS =
            VK_SUBGROUP_FEATURE_BASIC_BIT |
            VK_SUBGROUP_FEATURE_VOTE_BIT |
            VK_SUBGROUP_FEATURE_ARITHMETIC_BIT |
            VK_SUBGROUP_FEATURE_BALLOT_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT |
            VK_SUBGROUP_FEATURE_CLUSTERED_BIT |
            VK_SUBGROUP_FEATURE_QUAD_BIT;

        shaderSubgroupFullSupported = (subgroupProps.supportedOperations & REQUIRED_SUBGROUP_OPS) == REQUIRED_SUBGROUP_OPS;
    }

    enableDeviceFault = IsExtensionSupported(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    
    // Mesh Shader 的依赖扩展检测
    enableShaderFloatControls = IsExtensionSupported(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    enableSpirv14 = IsExtensionSupported(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

    // Mesh Shader 扩展检测（仅使用标准的 EXT 扩展，与 Metal 等其他图形 API 通用）
    // 需要 VK_KHR_spirv_1_4 和 VK_KHR_shader_float_controls 作为依赖
    enableMeshShaderEXT = IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME) &&
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

    // 查询时间线信号量的支持
    enableTimelineSemaphore = IsExtensionSupported(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    if (enableTimelineSemaphore)
    {
        VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {};
        timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        timelineSemaphoreFeatures.pNext = nullptr;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &timelineSemaphoreFeatures;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        enableTimelineSemaphore = timelineSemaphoreFeatures.timelineSemaphore == VK_TRUE;
    }

    enableSynchronization2 = IsExtensionSupported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    

#ifdef ENABLE_NSIGHT_AFTERMATH
    // NVIDIA Nsight Aftermath 扩展检测（仅 NVIDIA GPU 支持）
    if (physicalDeviceProperties.vendorID == 0x10DE) // NVIDIA
    {
        enableDiagnosticCheckpoints = IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
        enableDiagnosticsConfig = IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
    }
#endif

}  // VulkanExtension::Init

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
    enabledExtendedDynamicState = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) && 
        extendedDynamicStateFeaturesEXT.extendedDynamicState;
    enabledExtendedDynamicState2 = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) && 
        extendedDynamicState2FeaturesEXT.extendedDynamicState2;
    enabledExtendedDynamicState3 = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) && 
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

NAMESPACE_RENDERCORE_END
