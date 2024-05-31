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
    bool enabledExtendedDynamicState;
    bool enabledExtendedDynamicState2;
    bool enabledExtendedDynamicState3;
    
    std::vector<VkExtensionProperties> mDeviceExtensions;
    
    void Init(VkPhysicalDevice physicalDevice);
    
private:
    void InitExtendedDynamicState(VkPhysicalDevice physicalDevice);
    
    bool ExtensionSupported(const char* name);
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_EXTENSION_FSDJDSGJDFNBGDFJ_INCLUDE */
