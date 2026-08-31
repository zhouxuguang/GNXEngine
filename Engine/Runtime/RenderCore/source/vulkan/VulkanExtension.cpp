//
//  VulkanExtension.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/31.
//

#include "VulkanExtension.h"

NAMESPACE_RENDERCORE_BEGIN

void VulkanExtension::Init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties, 
        const DeviceExtFeature& deviceExtFeatures, const DeviceExtProperties& deviceExtProperties)
{
    mPhysicalDeviceProperties = physicalDeviceProperties;
    enableMaintenance3 = IsExtensionSupported(VK_KHR_MAINTENANCE_3_EXTENSION_NAME);
	InitExtendedDynamicState(deviceExtFeatures);

	enableDynamicRendering = IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) && 
        deviceExtFeatures.dynamicRenderingFeatures.dynamicRendering;
    enablePortabilitySubset = IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    enablePushDesDescriptor = IsExtensionSupported(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    enableDescriptorUpdateTemplate = IsExtensionSupported(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME);

    // host image copy关联的扩展
    enableFormatFeatureFlags2 = IsExtensionSupported(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
    enableCopyCommands2 = IsExtensionSupported(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
    InitHostImageCopy(deviceExtFeatures);
    
    // Subgroup 操作检测（Wave* 系列 HLSL 指令）
    // 保守策略：所有 core subgroup 操作都必须支持，waveIntrinsics 才返回 true
    // Core 操作: BASIC | VOTE | ARITHMETIC | BALLOT | SHUFFLE | SHUFFLE_RELATIVE | CLUSTERED | QUAD = 0xFF
    {
        static const VkSubgroupFeatureFlags REQUIRED_SUBGROUP_OPS =
            VK_SUBGROUP_FEATURE_BASIC_BIT |
            VK_SUBGROUP_FEATURE_VOTE_BIT |
            VK_SUBGROUP_FEATURE_ARITHMETIC_BIT |
            VK_SUBGROUP_FEATURE_BALLOT_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_BIT |
            VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT |
            VK_SUBGROUP_FEATURE_CLUSTERED_BIT |
            VK_SUBGROUP_FEATURE_QUAD_BIT;

        shaderSubgroupFullSupported = (deviceExtProperties.subgroupProperties.supportedOperations & REQUIRED_SUBGROUP_OPS) == REQUIRED_SUBGROUP_OPS;
    }

    enableDeviceFault = IsExtensionSupported(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    
    // Mesh Shader 的依赖扩展检测
    enableShaderFloatControls = IsExtensionSupported(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    enableSpirv14 = IsExtensionSupported(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

    // 需要 VK_KHR_spirv_1_4 和 VK_KHR_shader_float_controls 作为依赖
    enableMeshShaderEXT = IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME) &&
                          enableSpirv14 && enableShaderFloatControls && deviceExtFeatures.meshShaderFeatures.meshShader &&
        deviceExtFeatures.meshShaderFeatures.taskShader;

    // 查询时间线信号量的支持
    enableTimelineSemaphore = IsExtensionSupported(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) && deviceExtFeatures.timelineSemaphoreFeatures.timelineSemaphore;

    enableSynchronization2 = IsExtensionSupported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    // VK_EXT_astc_decode_mode：允许在创建 ASTC 纹理的 ImageView 时指定解码模式
    // （例如把 ASTC sRGB 数据按线性 UNORM 解码用于颜色计算）。
    // 该扩展存在即可启用（仅用于 ASTC LDR 的 sRGB/UNORM 互转，不需要额外特性）。
    enableAstcDecodeMode = IsExtensionSupported(VK_EXT_ASTC_DECODE_MODE_EXTENSION_NAME);
    // decodeModeSharedExponent 特性仅在把 ASTC HDR (SFLOAT) 解码为共享指数格式时需要，
    // 本引擎目前只处理 ASTC LDR，仅记录设备是否支持该能力。
    astcDecodeModeSharedExponent = deviceExtFeatures.astcDecodeFeatures.decodeModeSharedExponent == VK_TRUE;

    // 如下三个字段同时满足，才能启用 Bindless 特性
	enableDescriptorIndexing = IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) && enableMaintenance3 &&
	    deviceExtFeatures.descriptorIndexingFeatures.runtimeDescriptorArray && 
        deviceExtFeatures.descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount &&
        deviceExtFeatures.descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing;

    enableDrawIndirectCount = deviceExtFeatures.features12.drawIndirectCount == VK_TRUE;

#ifdef ENABLE_NSIGHT_AFTERMATH
    // NVIDIA Nsight Aftermath 扩展检测（仅 NVIDIA GPU 支持）
    if (physicalDeviceProperties.vendorID == 0x10DE) // NVIDIA
    {
        enableDiagnosticCheckpoints = IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
        enableDiagnosticsConfig = IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
    }
#endif

}  // VulkanExtension::Init

void VulkanExtension::InitExtendedDynamicState(const DeviceExtFeature& deviceExtFeature)
{
    enableExtendedDynamicState = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) && 
        deviceExtFeature.extendedDynamicStateFeaturesEXT.extendedDynamicState;
    enableExtendedDynamicState2 = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) && 
        deviceExtFeature.extendedDynamicState2FeaturesEXT.extendedDynamicState2;
    enableExtendedDynamicState3 = IsExtensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) && 
        deviceExtFeature.extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable && 
        deviceExtFeature.extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation;
}

void VulkanExtension::InitHostImageCopy(const DeviceExtFeature &deviceExtFeature)
{
    enableHostImageCopy = IsExtensionSupported(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME) &&
        enableFormatFeatureFlags2 && enableCopyCommands2 && (deviceExtFeature.hostImageCopyFeatures.hostImageCopy == VK_TRUE);

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
