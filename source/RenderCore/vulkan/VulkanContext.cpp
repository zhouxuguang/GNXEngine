//
//  VulkanContext.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanContext.h"
#include "VulkanDeviceUtil.h"
#include "BaseLib/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

bool CreateInstance(VulkanContext& context, uint32_t apiVersion)
{
    std::vector<const char *> instanceExtensions;

    instanceExtensions.push_back("VK_KHR_surface");
#ifdef __ANDROID__
    instanceExtensions.push_back("VK_KHR_android_surface");
#else define __APPLE__
    instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.apiVersion = apiVersion;
    appInfo.applicationVersion = apiVersion;
    appInfo.engineVersion = apiVersion;
    appInfo.pApplicationName = "GNXEngine";
    appInfo.pEngineName = "GNXEngine";
    
    std::vector<const char *> instanceLayers;

    // 创建vulkan实例
    VkInstanceCreateInfo instanceCreateInfo = {};
#ifdef __APPLE__
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = (uint32_t)(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instance);
    if (result != VK_SUCCESS) 
    {
        return false;
    }
    return true;
}

bool SelectPhysicalDevice(VulkanContext& context)
{
    bool ret = false;
    std::vector<VkPhysicalDevice> gpuDevices;
    VulkanDeviceUtil::GetPhysicalDevices(context.instance, gpuDevices);
    if (gpuDevices.empty())
    {
        return ret;
    }

    for (uint32_t i = 0; i < gpuDevices.size(); ++i)
    {
        VkPhysicalDevice physicalDevice = gpuDevices[i];
        vkGetPhysicalDeviceProperties(physicalDevice, &context.physicalDeviceProperties);
        context.numSamples = VulkanDeviceUtil::GetMaxUsableSampleCount(context.physicalDeviceProperties);

        //判断device是否有支持graphics的comand queues.
        uint32_t queueFamiliesCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
        if (queueFamiliesCount == 0) 
        {
            continue;
        }
        std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesProperties.data());
        context.graphicsQueueFamilyIndex = 0xffffffff;
        for (uint32_t j = 0; j < queueFamiliesCount; ++j)
        {
            const VkQueueFamilyProperties& props = queueFamiliesProperties[j];
            if (props.queueCount == 0) 
            {
                continue;
            }
            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                context.graphicsQueueFamilyIndex = j;
                context.queueCount = props.queueCount;
            }
        }
        if (context.graphicsQueueFamilyIndex == 0xffffffff) continue;

        //判断device是否支持 VK_KHR_swapchain extension
        uint32_t extensionCount;
        VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        if (VK_SUCCESS != result)
        {
            printf("vkEnumerateDeviceExtensionProperties count error\n");
            return ret;
        }
       
        std::vector<VkExtensionProperties> extensions(extensionCount);
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
        if (VK_SUCCESS != result)
        {
            printf("vkEnumerateDeviceExtensionProperties count error 1\n");
            return ret;
        }
        
        for (const auto &iter : extensions)
        {
            context.extensionNames.push_back(iter.extensionName);
        }
        
        bool supportsSwapchain = false;
        context.debugMarkersSupported = false;
        for (uint32_t k = 0; k < extensionCount; ++k) 
        {
            if (!strcmp(context.extensionNames[k], VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            {
                supportsSwapchain = true;
            }
            if (!strcmp(context.extensionNames[k], VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
            {
                context.debugMarkersSupported = true;
            }
        }
        if (!supportsSwapchain) continue;

        //最终找打我们需要的physical device
        context.physicalDevice = physicalDevice;
        vkGetPhysicalDeviceFeatures(physicalDevice, &context.physicalDeviceFeatures);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &context.memoryProperties);
        ret = true;
        break;
    }
    return ret;
}

static VKAPI_PTR VkBool32 DebugReportCallback(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData)
{
    const char validation[]  = "Validation";
    const char performance[] = "Performance";
    const char error[]       = "ERROR";
    const char warning[]     = "WARNING";
    const char unknownType[] = "UNKNOWN_TYPE";
    const char unknownSeverity[] = "UNKNOWN_SEVERITY";
    const char* typeString      = unknownType;
    const char* severityString  = unknownSeverity;
    const char* messageIdName   = pLayerPrefix;
    int32_t messageIdNumber     = messageCode;
    const char* message         = pMessage;
    

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
    {
        severityString = error;
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) 
    {
        severityString = warning;
    }
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) 
    {
        typeString = validation;
    }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) 
    {
        typeString = performance;
    }
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) 
    {
        typeString = performance;
    }

    log_info(
                        "%s %s: [%s] Code %i : %s",
                        typeString,
                        severityString,
                        messageIdName,
                        messageIdNumber,
                        message);

    // Returning false tells the layer not to stop when the event occurs, so
    // they see the same behavior with and without validation layers enabled.
    return VK_FALSE;
}

static void CreateDebugReport(VulkanContext& context)
{
    if (vkCreateDebugReportCallbackEXT)
    {
        VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfoExt;

        debugReportCallbackCreateInfoExt.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        debugReportCallbackCreateInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugReportCallbackCreateInfoExt.pNext = nullptr;
        debugReportCallbackCreateInfoExt.pfnCallback = DebugReportCallback;
        debugReportCallbackCreateInfoExt.pUserData = nullptr;

        VkDebugReportCallbackEXT debugReportCallbackExt;
        vkCreateDebugReportCallbackEXT(context.instance, &debugReportCallbackCreateInfoExt, nullptr, &debugReportCallbackExt);
        context.debugReportCallbackExt = debugReportCallbackExt;
    }
}

bool CreateVirtualDevice(VulkanContext& context)
{
    VkDeviceQueueCreateInfo deviceQueueCreateInfo[1] = {};

    VkDeviceCreateInfo deviceCreateInfo = {};
    std::vector<const char*> deviceExtensionNames;
    deviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceExtensionNames.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

//#ifdef ENABLE_VULKAN_DEBUG
//    if (context.debugMarkersSupported) 
//    {
//        deviceExtensionNames.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
//    }
//    CreateDebugReport(context);
//#endif
    
    constexpr VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = nullptr,
        .dynamicRendering = VK_TRUE,
    };

    deviceQueueCreateInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo->pNext = &dynamicRenderingFeature;
    deviceQueueCreateInfo->queueFamilyIndex = context.graphicsQueueFamilyIndex;
    std::vector<float> queuePriority;
    for (int i = 0; i < context.queueCount; ++i)
    {
        queuePriority.push_back(1.0f);
    }
    deviceQueueCreateInfo->queueCount = context.queueCount;
    deviceQueueCreateInfo->pQueuePriorities = queuePriority.data();
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;

    const auto& supportedFeatures = context.physicalDeviceFeatures;
    
    VkPhysicalDeviceFeatures enabledFeatures = {};
    enabledFeatures.textureCompressionETC2 = supportedFeatures.textureCompressionETC2;
    enabledFeatures.textureCompressionBC = supportedFeatures.textureCompressionBC;

    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensionNames.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
    VkResult result = vkCreateDevice(context.physicalDevice, &deviceCreateInfo, NULL, &context.device);

    if (result != VK_SUCCESS) 
    {
        log_info("vkCreateDevice error");
        return false;
    }
    
    vkGetDeviceQueue(context.device, context.graphicsQueueFamilyIndex, 0, &context.graphicsQueue);
    
    //创建命令缓冲区池
    VkCommandPoolCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = context.graphicsQueueFamilyIndex;
    createInfo.pNext = nullptr;
    
    result = vkCreateCommandPool(context.device, &createInfo, NULL, &context.commandPool);
    if (result != VK_SUCCESS) 
    {
        log_info("vkCreateCommandPool error");
        return false;
    }

    return true;
}

// 创建内存分配器
static void GetVulkanFunctions(VmaVulkanFunctions &vulkanFunctions)
{
    vulkanFunctions.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetDeviceProcAddr;
    
#if 1
    vulkanFunctions.vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetPhysicalDeviceMemoryProperties;
    vulkanFunctions.vkAllocateMemory = (PFN_vkAllocateMemory)vkAllocateMemory;
    vulkanFunctions.vkFreeMemory = (PFN_vkFreeMemory)vkFreeMemory;
    vulkanFunctions.vkMapMemory = (PFN_vkMapMemory)vkMapMemory;
    vulkanFunctions.vkUnmapMemory = (PFN_vkUnmapMemory)vkUnmapMemory;
    vulkanFunctions.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkFlushMappedMemoryRanges;
    vulkanFunctions.vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)vkInvalidateMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory = (PFN_vkBindBufferMemory)vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory = (PFN_vkBindImageMemory)vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer = (PFN_vkCreateBuffer)vkCreateBuffer;
    vulkanFunctions.vkDestroyBuffer = (PFN_vkDestroyBuffer)vkDestroyBuffer;
    vulkanFunctions.vkCreateImage = (PFN_vkCreateImage)vkCreateImage;
    vulkanFunctions.vkDestroyImage = (PFN_vkDestroyImage)vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vkCmdCopyBuffer;

    vulkanFunctions.vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)vkGetBufferMemoryRequirements2KHR;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)vkGetImageMemoryRequirements2KHR;
    vulkanFunctions.vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)vkBindBufferMemory2KHR;
    vulkanFunctions.vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)vkBindImageMemory2KHR;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)vkGetPhysicalDeviceMemoryProperties2KHR;

    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
}

void CreateVMA(VulkanContext& context)
{
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo = {0};
    vmaAllocatorCreateInfo.device = context.device;
    vmaAllocatorCreateInfo.physicalDevice = context.physicalDevice;
    vmaAllocatorCreateInfo.instance = context.instance;
    vmaAllocatorCreateInfo.flags = 0;
    vmaAllocatorCreateInfo.pAllocationCallbacks = nullptr;
    vmaAllocatorCreateInfo.pDeviceMemoryCallbacks = nullptr;
    vmaAllocatorCreateInfo.preferredLargeHeapBlockSize = 128 * 1024 * 1024;
    vmaAllocatorCreateInfo.pHeapSizeLimit = nullptr;
    vmaAllocatorCreateInfo.vulkanApiVersion = context.apiVersion;
    vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    VmaVulkanFunctions vmaVulkanFunctions;
    GetVulkanFunctions(vmaVulkanFunctions);
    vmaAllocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

    vmaCreateAllocator(&vmaAllocatorCreateInfo, &context.vmaAllocator);
}

bool CreateSurfaceKHR(VulkanContext& context, ViewHandle nativeWidow)
{
    if (nullptr == nativeWidow)
    {
        return false;
    }

#ifdef VK_USE_PLATFORM_MACOS_MVK
    VkMacOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pView = nativeWidow;
    PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(context.instance, "vkCreateMacOSSurfaceMVK");
    VkResult result = vkCreateMacOSSurfaceMVK(context.instance, &createInfo, nullptr, &context.surfaceKhr);
#endif

    return context.surfaceKhr != VK_NULL_HANDLE;
}

NAMESPACE_RENDERCORE_END