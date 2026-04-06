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
    
    // Mesh Shader 扩展支持
    bool enableMeshShaderEXT = false;
    bool enableMeshShaderNV = false;
    
    std::vector<VkExtensionProperties> mDeviceExtensions;
    
    void Init(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties physicalDeviceProperties);
    
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
