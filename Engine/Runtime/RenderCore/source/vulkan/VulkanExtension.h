//
//  VulkanExtension.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/31.
//

#ifndef GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE
#define GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE

#include "VKRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

struct VulkanExtension
{
    bool enabledExtendedDynamicState = false;
    bool enabledExtendedDynamicState2 = false;
    bool enabledExtendedDynamicState3 = false;
    
    bool enabledDynamicRendering = false;
    bool enablePushDesDescriptor = false;
    bool enableDescriptorUpdateTemplate = false;
    bool enableDebugUtils = false;
    bool enableFormatFeatureFlags2 = false;
    bool enableCopyCommands2 = false;
    bool enableHostImageCopy = false;
    bool enableDeviceFault = false;
    bool enablePortabilitySubset = false;
    
    // Mesh Shader 扩展支持（仅使用标准的 EXT 扩展）
    bool enableMeshShaderEXT = false;

    // Mesh Shader 的依赖扩展
    bool enableSpirv14 = false;
    bool enableShaderFloatControls = false;

    // Mesh Shader 实际 feature 支持情况（通过 vkGetPhysicalDeviceFeatures2 查询）
    bool meshShaderSupported = false;
    bool taskShaderSupported = false;

    // drawIndirectCount 特性支持（Vulkan 1.2 core feature）
    bool enableDrawIndirectCount = false;

    // NVIDIA Nsight Aftermath 支持
    bool enableAftermath = false;              // Aftermath SDK 初始化成功
    bool enableDiagnosticCheckpoints = false;   // VK_NV_device_diagnostic_checkpoints 扩展
    bool enableDiagnosticsConfig = false;       // VK_NV_device_diagnostics_config 扩展

    // Subgroup 全操作支持（Wave* 系列 HLSL 指令）
    // 保守策略：所有 core subgroup 操作（BASIC|VOTE|ARITHMETIC|BALLOT|SHUFFLE|SHUFFLE_RELATIVE|CLUSTERED|QUAD）
    // 都支持时才为 true，上层 waveIntrinsics 依赖此标志
    // Vulkan 1.2+: core feature，无需扩展
    // Vulkan 1.1:  需启用 VK_EXT_shader_subgroup_ballot 等扩展
    bool shaderSubgroupFullSupported = false;

    // 时间线信号量
    bool enableTimelineSemaphore = false;

    // 同步2扩展
    bool enableSynchronization2 = false;

    std::vector<VkExtensionProperties> mDeviceExtensions;
    
    void Init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties);

    /// 检查指定扩展是否被设备支持（public 接口，供 RenderDeviceFeatures 初始化使用）
    bool IsExtensionSupported(const char* name) const;

private:
    void InitExtendedDynamicState(VkPhysicalDevice physicalDevice);
    
    bool ExtensionSupported(const char* name);

private:
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
};

struct VulkanFeature
{
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE */
