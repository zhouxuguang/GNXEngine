//
//  VulkanExtension.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/31.
//

#ifndef GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE
#define GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE

#include "VKRenderDefine.h"
#include "VulkanFeatures.h"

NAMESPACE_RENDERCORE_BEGIN

struct VulkanExtension
{
    bool enableMaintenance3 = false;
    bool enableExtendedDynamicState = false;
    bool enableExtendedDynamicState2 = false;
    bool enableExtendedDynamicState3 = false;
    
    bool enableDynamicRendering = false;
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

    // drawIndirectCount 特性支持（Vulkan 1.2 core feature）
    bool enableDrawIndirectCount = false;

    // NVIDIA Nsight Aftermath 支持
    bool enableAftermath = false;              // Aftermath SDK 初始化成功
    bool enableDiagnosticCheckpoints = false;   // VK_NV_device_diagnostic_checkpoints 扩展
    bool enableDiagnosticsConfig = false;       // VK_NV_device_diagnostics_config 扩展

    bool shaderSubgroupFullSupported = false;

    // 时间线信号量
    bool enableTimelineSemaphore = false;

    // 同步2扩展
    bool enableSynchronization2 = false;

    bool enableDescriptorIndexing = false;

    std::vector<VkExtensionProperties> mDeviceExtensions;
    
    void Init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties,
        const DeviceExtFeature& deviceExtFeatures, const DeviceExtProperties& deviceExtProperties);

    /// 检查指定扩展是否被设备支持（public 接口，供 RenderDeviceFeatures 初始化使用）
    bool IsExtensionSupported(const char* name) const;

private:
    void InitExtendedDynamicState(const DeviceExtFeature& deviceExtFeature);

    void InitHostImageCopy(const DeviceExtFeature& deviceExtFeature);

private:
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
};

struct VulkanFeature
{
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE */
