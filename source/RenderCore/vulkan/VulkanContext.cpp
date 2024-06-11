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

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        printf("{%d} - {%s}: {%s}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("{%d} - {%s}: {%s}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    return VK_FALSE;
}

bool CreateInstance(VulkanContext& context, uint32_t apiVersion)
{
    std::vector<const char *> instanceExtensions;

    instanceExtensions.push_back("VK_KHR_surface");
#ifdef __ANDROID__
    instanceExtensions.push_back("VK_KHR_android_surface");
#elif __APPLE__
    instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
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
    
    uint32_t instance_extension_count;
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);

    std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data());
    
    bool debug_utils = false;
    for (auto &available_extension : available_instance_extensions)
    {
        if (strcmp(available_extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
        {
            debug_utils = true;
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (strcmp(available_extension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
        {
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }
    
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

    debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

    // 创建vulkan实例
    VkInstanceCreateInfo instanceCreateInfo = {};
#ifdef __APPLE__
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = &debug_utils_create_info;
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
    
    // 加载所有instance的入口点
    volkLoadInstanceOnly(context.instance);
    
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    result = vkCreateDebugUtilsMessengerEXT(context.instance, &debug_utils_create_info, nullptr, &debug_utils_messenger);
    
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
            printf("%s\n", iter.extensionName);
        }
        
        const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        
        VkFormatProperties formatProperties;
        // Get device properties for the requested texture format
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
        // Check if requested image format supports image storage operations required for storing pixel from the compute shader
        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
        
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
        
        context.vulkanExtension.mDeviceExtensions.swap(extensions);

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
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugReportCallbackEXT");
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugReportCallbackEXT");
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(context.instance, "vkDebugReportMessageEXT");
    
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
    if (context.vulkanExtension.enabledDynamicRendering)
    {
        deviceExtensionNames.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    }

#ifdef ENABLE_VULKAN_DEBUG
    if (context.debugMarkersSupported) 
    {
        deviceExtensionNames.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
    if (context.debugReportCallbackExt)
    {
        deviceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    CreateDebugReport(context);
#endif
    
    void *deviceCreatepNextChain = nullptr;
    
    // 扩展动态状态
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
    vkGetPhysicalDeviceFeatures2(context.physicalDevice, &physicalDeviceFeatures2);
    
    if (context.vulkanExtension.enabledExtendedDynamicState)
    {
        deviceExtensionNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        extendedDynamicStateFeaturesEXT.pNext = nullptr;
        deviceCreatepNextChain = &extendedDynamicStateFeaturesEXT;
    }
    if (context.vulkanExtension.enabledExtendedDynamicState2)
    {
        deviceExtensionNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        extendedDynamicState2FeaturesEXT.pNext = nullptr;
        if (context.vulkanExtension.enabledExtendedDynamicState)
        {
            extendedDynamicStateFeaturesEXT.pNext = &extendedDynamicState2FeaturesEXT;
        }
        else
        {
            deviceCreatepNextChain = &extendedDynamicState2FeaturesEXT;
        }
    }
    if (context.vulkanExtension.enabledExtendedDynamicState3)
    {
        deviceExtensionNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
        if (context.vulkanExtension.enabledExtendedDynamicState2)
        {
            extendedDynamicState2FeaturesEXT.pNext = &extendedDynamicState3FeaturesEXT;
        }
        else
        {
            deviceCreatepNextChain = &extendedDynamicState3FeaturesEXT;
        }
    }
    
    // 打开 VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME    
    // push descriptor默认开启
    deviceExtensionNames.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    
    const VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = deviceCreatepNextChain,
        .dynamicRendering = VK_TRUE,
    };
    
    // 开启负的高度
    deviceExtensionNames.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

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
    
    volkLoadDevice(context.device);
    
    vkGetDeviceQueue(context.device, context.graphicsQueueFamilyIndex, 0, &context.graphicsQueue);

    return true;
}

VkCommandPool VulkanContext::GetCommandPool()
{
    void *pCommandPool = nullptr;
    if ((pCommandPool = commandPoolTls.get()))
    {
        return (VkCommandPool)pCommandPool;
    }
    
    else
    {
        //创建命令缓冲区池
        VkCommandPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
        createInfo.pNext = nullptr;
        
        VkCommandPool commandPool = VK_NULL_HANDLE;
        
        VkResult result = vkCreateCommandPool(device, &createInfo, NULL, &commandPool);
        if (result != VK_SUCCESS)
        {
            log_info("vkCreateCommandPool error");
        }
        
        commandPoolTls.set(commandPool);
        return commandPool;
    }
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

#ifdef VK_USE_PLATFORM_METAL_EXT
    VkMetalSurfaceCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    createInfo.pLayer = (const CAMetalLayer*)nativeWidow;
    PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(context.instance, "vkCreateMetalSurfaceEXT");
    VkResult result = vkCreateMetalSurfaceEXT(context.instance, &createInfo, nullptr, &context.surfaceKhr);
#endif

    return context.surfaceKhr != VK_NULL_HANDLE;
}

void CreateComputeDescriptorPool(VulkanContext& context)
{
    constexpr int maxInflight = 3;
    // 创建VkDescriptorPool
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = maxInflight;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = maxInflight;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = maxInflight;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
    VkResult res = vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &context.computeDescriptorPool);
    if (res != VK_SUCCESS)
    {
        printf("vkCreateDescriptorPool failed!!!!\n");
    }
}

VkDescriptorSet AllocDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descLayout)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descLayout;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    return descriptorSet;
}

NAMESPACE_RENDERCORE_END
