//
//  VulkanContext.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanContext.h"
#include "VulkanDeviceUtil.h"
#include "BaseLib/LogService.h"
#include "VKUtil.h"
#include <optional>

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

static const std::vector<const char*> validationLayers = 
{
	"VK_LAYER_KHRONOS_validation"
};

static bool checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) 
    {
		bool layerFound = false;

        //检查所有的layer在可以使用的layer中
		for (const auto& layerProperties : availableLayers) 
        {
			if (strcmp(layerName, layerProperties.layerName) == 0) 
            {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) 
        {
			return false;
		}
	}

	return true;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debug_utils_messenger_callback;
}

bool CreateInstance(VulkanContext& context, uint32_t apiVersion)
{
    std::vector<const char *> instanceExtensions;

    instanceExtensions.push_back("VK_KHR_surface");
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    instanceExtensions.push_back("VK_KHR_android_surface");
#elif defined VK_USE_PLATFORM_METAL_EXT
    instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#elif defined VK_USE_PLATFORM_WIN32_KHR
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.apiVersion = apiVersion;
    appInfo.applicationVersion = apiVersion;
    appInfo.engineVersion = apiVersion;
    appInfo.pApplicationName = "GNXEngine";
    appInfo.pEngineName = "GNXEngine";
    
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
    
    // 创建vulkan实例
    VkInstanceCreateInfo instanceCreateInfo = {};

    // 检查vulkan验证层的情况
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	if (context.enableValidationLayers && checkValidationLayerSupport())
    {
        instanceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        
        context.vulkanExtension.enableDebugUtils = true;
	}
	else 
    {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
        context.enableValidationLayers = false;
        context.vulkanExtension.enableDebugUtils = false;
	}

#ifdef __APPLE__
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = (uint32_t)(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instance);
    if (result != VK_SUCCESS) 
    {
        return false;
    }
    
    // 加载所有instance的入口点
    volkLoadInstanceOnly(context.instance);
    
    if (context.enableValidationLayers)
    {
        result = vkCreateDebugUtilsMessengerEXT(context.instance, &debugCreateInfo, nullptr, &context.debugUtilsMessenger);
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
        context.queueFamiliesProperties.resize(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, context.queueFamiliesProperties.data());
        context.graphicsQueueFamilyIndex = 0xffffffff;

        for (uint32_t j = 0; j < queueFamiliesCount; ++j)
        {
            const VkQueueFamilyProperties& props = context.queueFamiliesProperties[j];
            if (props.queueCount == 0) 
            {
                continue;
            }
            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            {
                context.graphicsQueueFamilyIndex = j;
                context.queueCount = props.queueCount;
            }

			if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// 图形队列族
                //graphicsQueueFamilyIndex = j;
			}
			else if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				// 具有单独的计算队列族
                //computeQueueFamilyIndex = j;
			}
			else if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				// 具有单独的传输队列族
                context.transferQueueFamilyIndex = j;
                context.transferQueueFamilyIndex = 1;
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

bool CreateVirtualDevice(VulkanContext& context)
{
    VkDeviceCreateInfo deviceCreateInfo = {};
    std::vector<const char*> deviceExtensionNames;
    deviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (context.vulkanExtension.enabledDynamicRendering)
    {
        deviceExtensionNames.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    }

    deviceExtensionNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

#ifdef ENABLE_VULKAN_DEBUG
    if (context.debugMarkersSupported) 
    {
        deviceExtensionNames.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
    if (context.debugReportCallbackExt)
    {
        deviceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
#endif
    
    // 扩展动态状态
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT = {};
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT = {};
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT = {};
    
    // 获得完整的扩展动态状态
    extendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    AppendToPNextChain(&extendedDynamicStateFeaturesEXT, &extendedDynamicState2FeaturesEXT);
  
    extendedDynamicState2FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    AppendToPNextChain(&extendedDynamicState2FeaturesEXT, &extendedDynamicState3FeaturesEXT);

    extendedDynamicState3FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

    // 查询VkPhysicalDeviceDescriptorIndexingFeatures
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    //AppendToPNextChain(&extendedDynamicState3FeaturesEXT, &indexingFeatures);

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    AppendToPNextChain(&physicalDeviceFeatures2, &extendedDynamicStateFeaturesEXT);
    vkGetPhysicalDeviceFeatures2(context.physicalDevice, &physicalDeviceFeatures2);

	if (!indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind) 
    {
		// 设备不支持，需要处理回退逻辑或报错
        log_info("descriptorBindingUniformBufferUpdateAfterBind NOT SUPPORTED");
	}
    
    // 构建设备创建时的开启的特性
    void* deviceCreateNextChain = nullptr;
    if (context.vulkanExtension.enabledExtendedDynamicState)
    {
        deviceExtensionNames.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        extendedDynamicStateFeaturesEXT.pNext = nullptr;
        deviceCreateNextChain = &extendedDynamicStateFeaturesEXT;
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
            deviceCreateNextChain = &extendedDynamicState2FeaturesEXT;
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
            deviceCreateNextChain = &extendedDynamicState3FeaturesEXT;
        }
    }

    //indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;

    //deviceExtensionNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    
    // 打开 VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME    
    // push descriptor开启
    if (context.vulkanExtension.enablePushDesDescriptor)
    {
        deviceExtensionNames.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    }
    
    if (context.vulkanExtension.enableDeviceFault)
    {
        deviceExtensionNames.push_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    }
    
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature = {};
	dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeature.pNext = deviceCreateNextChain;
	dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    context.features_11.pNext = &context.features_12;
    context.features_12.pNext = &dynamicRenderingFeature;
    
    // 开启负的高度
    deviceExtensionNames.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

    deviceExtensionNames.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    deviceExtensionNames.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    deviceExtensionNames.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);

    VkPhysicalDeviceHostImageCopyFeaturesEXT host_image_copy_features = {};
    if (context.vulkanExtension.enableHostImageCopy)
    {
		host_image_copy_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT;
		host_image_copy_features.hostImageCopy = VK_TRUE;

        AppendToPNextChain(&context.features_11, &host_image_copy_features);

        deviceExtensionNames.push_back(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
        deviceExtensionNames.push_back(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
        deviceExtensionNames.push_back(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME);
    }

    // 队列优先级的属性
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

    // 图形队列
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = context.graphicsQueueFamilyIndex;
    std::vector<float> queuePriority;
    for (int i = 0; i < context.queueCount; ++i)
    {
        queuePriority.push_back(1.0f);
    }
    deviceQueueCreateInfo.queueCount = context.queueCount;
    deviceQueueCreateInfo.pQueuePriorities = queuePriority.data();
    deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

    // 传输队列
    uint32_t transferQueueCount = context.queueFamiliesProperties[context.transferQueueFamilyIndex].queueCount;
	deviceQueueCreateInfo.queueFamilyIndex = context.transferQueueFamilyIndex;
	std::vector<float> transQueuePriority;
	for (int i = 0; i < transferQueueCount; ++i)
	{
        transQueuePriority.push_back(0.95f);
	}
	deviceQueueCreateInfo.queueCount = transferQueueCount;
	deviceQueueCreateInfo.pQueuePriorities = transQueuePriority.data();
    deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &context.features_11;
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)deviceQueueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();

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
    
    // 获得图形队列
    vkGetDeviceQueue(context.device, context.graphicsQueueFamilyIndex, 0, &context.graphicsQueue);

    // 获得传输队列
    context.availableTransferQueues.resize(transferQueueCount);
    for (uint32_t i = 0; i < transferQueueCount; i ++)
    {
        vkGetDeviceQueue(context.device, context.transferQueueFamilyIndex, i, context.availableTransferQueues.data() + i);
    }

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

VkCommandPool VulkanContext::GetTransferCommandPool()
{
	void* pCommandPool = nullptr;
	if ((pCommandPool = transferCommandPoolTls.get()))
	{
		return (VkCommandPool)pCommandPool;
	}

	else
	{
		//创建命令缓冲区池
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = transferQueueFamilyIndex;
		createInfo.pNext = nullptr;

		VkCommandPool commandPool = VK_NULL_HANDLE;

		VkResult result = vkCreateCommandPool(device, &createInfo, NULL, &commandPool);
		if (result != VK_SUCCESS)
		{
			log_info("vkCreateCommandPool error");
		}

        transferCommandPoolTls.set(commandPool);
		return commandPool;
	}
}

// 创建内存分配器
static void GetVulkanFunctions(VkDevice device, VmaVulkanFunctions &vulkanFunctions)
{
    vulkanFunctions.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetDeviceProcAddr;
    
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

    vulkanFunctions.vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR");
    vulkanFunctions.vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2KHR");;
    vulkanFunctions.vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)vkGetDeviceProcAddr(device, "vkBindBufferMemory2KHR");;
    vulkanFunctions.vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)vkGetDeviceProcAddr(device, "vkBindImageMemory2KHR");;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)vkGetDeviceProcAddr(device, "vkGetPhysicalDeviceMemoryProperties2KHR");;
}

void CreateVMA(VulkanContext& context)
{
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo = {0};
    vmaAllocatorCreateInfo.device = context.device;
    vmaAllocatorCreateInfo.physicalDevice = context.physicalDevice;
    vmaAllocatorCreateInfo.instance = context.instance;
    vmaAllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT | VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
    vmaAllocatorCreateInfo.flags = 0;
    vmaAllocatorCreateInfo.pAllocationCallbacks = nullptr;
    vmaAllocatorCreateInfo.pDeviceMemoryCallbacks = nullptr;
    //vmaAllocatorCreateInfo.preferredLargeHeapBlockSize = 128 * 1024 * 1024;
    vmaAllocatorCreateInfo.pHeapSizeLimit = nullptr;
    vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    VmaVulkanFunctions vmaVulkanFunctions;
    GetVulkanFunctions(context.device, vmaVulkanFunctions);
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
#elif defined VK_USE_PLATFORM_WIN32_KHR
    VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = (HWND)nativeWidow;
	createInfo.hinstance = GetModuleHandle(nullptr);
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(context.instance, "vkCreateWin32SurfaceKHR");
    VkResult result = vkCreateWin32SurfaceKHR(context.instance, &createInfo, nullptr, &context.surfaceKhr);
#endif

    return context.surfaceKhr != VK_NULL_HANDLE;
}

void CreateGraphicsDescriptorPool(VulkanContext& context)
{
	constexpr int maxCount = 16;
    constexpr int maxSetCount = 10000;
	// 创建VkDescriptorPool

	VkDescriptorPoolSize poolSizes[4] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSizes[0].descriptorCount = maxCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSizes[1].descriptorCount = maxCount;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = maxCount;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[3].descriptorCount = maxCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = 4;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = maxSetCount;

	VkResult res = vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &context.graphicsDescriptorPool);
	if (res != VK_SUCCESS)
	{
		printf("vkCreateDescriptorPool failed!!!!\n");
	}
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

void ChoosePhysicalDevice(PFN_vkGetPhysicalDeviceProperties2 pGetPhysicalDeviceProperties2,
	const std::vector<VkPhysicalDevice>& physicalDevices,
	uint32_t preferredVendorID,
	uint32_t preferredDeviceID,
	const uint8_t* preferredDeviceUUID,
	const uint8_t* preferredDriverUUID,
	VkDriverId preferredDriverID,
	VkPhysicalDevice* physicalDeviceOut,
	VkPhysicalDeviceProperties2* physicalDeviceProperties2Out,
	VkPhysicalDeviceIDProperties* physicalDeviceIDPropertiesOut,
	VkPhysicalDeviceDriverProperties* physicalDeviceDriverPropertiesOut)
{
	VkPhysicalDeviceProperties const* deviceProps = &physicalDeviceProperties2Out->properties;

	const bool shouldChooseByPciId = (preferredVendorID != 0 || preferredDeviceID != 0);
	const bool shouldChooseByUUIDs = (preferredDeviceUUID != nullptr ||
		preferredDriverUUID != nullptr || preferredDriverID != 0);

	for (const VkPhysicalDevice& physicalDevice : physicalDevices)
	{
		*physicalDeviceProperties2Out = {};
		physicalDeviceProperties2Out->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProperties2Out->pNext = physicalDeviceIDPropertiesOut;

		*physicalDeviceIDPropertiesOut = {};
		physicalDeviceIDPropertiesOut->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
		physicalDeviceIDPropertiesOut->pNext = physicalDeviceDriverPropertiesOut;

		*physicalDeviceDriverPropertiesOut = {};
		physicalDeviceDriverPropertiesOut->sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

		pGetPhysicalDeviceProperties2(physicalDevice, physicalDeviceProperties2Out);

		if (deviceProps->apiVersion < VK_MAKE_API_VERSION(0, 1, 1, 0))
		{
			// Skip any devices that don't support our minimum API version. This
			// takes precedence over all other considerations.
			continue;
		}

		if (shouldChooseByUUIDs)
		{
			bool matched = true;

			if (preferredDriverID != 0 &&
				preferredDriverID != physicalDeviceDriverPropertiesOut->driverID)
			{
				matched = false;
			}
			else if (preferredDeviceUUID != nullptr &&
				memcmp(preferredDeviceUUID, physicalDeviceIDPropertiesOut->deviceUUID,
					VK_UUID_SIZE) != 0)
			{
				matched = false;
			}
			else if (preferredDriverUUID != nullptr &&
				memcmp(preferredDriverUUID, physicalDeviceIDPropertiesOut->driverUUID,
					VK_UUID_SIZE) != 0)
			{
				matched = false;
			}

			if (matched)
			{
				*physicalDeviceOut = physicalDevice;
				return;
			}
		}

		if (shouldChooseByPciId)
		{
			// NOTE: If the system has multiple GPUs with the same vendor and
			// device IDs, this will arbitrarily select one of them.
			bool matchVendorID = true;
			bool matchDeviceID = true;

			if (preferredVendorID != 0 && preferredVendorID != deviceProps->vendorID)
			{
				matchVendorID = false;
			}

			if (preferredDeviceID != 0 && preferredDeviceID != deviceProps->deviceID)
			{
				matchDeviceID = false;
			}

			if (matchVendorID && matchDeviceID)
			{
				*physicalDeviceOut = physicalDevice;
				return;
			}
		}
	}

	std::optional<VkPhysicalDevice> integratedDevice;
	VkPhysicalDeviceProperties2 integratedDeviceProperties2;
	VkPhysicalDeviceIDProperties integratedDeviceIDProperties;
	VkPhysicalDeviceDriverProperties integratedDeviceDriverProperties;

	for (const VkPhysicalDevice& physicalDevice : physicalDevices)
	{
		*physicalDeviceProperties2Out = {};
		physicalDeviceProperties2Out->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProperties2Out->pNext = physicalDeviceIDPropertiesOut;

		*physicalDeviceIDPropertiesOut = {};
		physicalDeviceIDPropertiesOut->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
		physicalDeviceIDPropertiesOut->pNext = physicalDeviceDriverPropertiesOut;

		*physicalDeviceDriverPropertiesOut = {};
		physicalDeviceDriverPropertiesOut->sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

		pGetPhysicalDeviceProperties2(physicalDevice, physicalDeviceProperties2Out);

		if (deviceProps->apiVersion < VK_MAKE_API_VERSION(0, 1, 1, 0))
		{
			// Skip any devices that don't support our minimum API version. This
			// takes precedence over all other considerations.
			continue;
		}

		// If discrete GPU exists, uses it by default.
		if (deviceProps->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			*physicalDeviceOut = physicalDevice;
			return;
		}
		if (deviceProps->deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
			!integratedDevice.has_value())
		{
			integratedDevice = physicalDevice;
			integratedDeviceProperties2 = *physicalDeviceProperties2Out;
			integratedDeviceIDProperties = *physicalDeviceIDPropertiesOut;
			integratedDeviceDriverProperties = *physicalDeviceDriverPropertiesOut;
			integratedDeviceProperties2.pNext = nullptr;
			integratedDeviceIDProperties.pNext = nullptr;
			integratedDeviceDriverProperties.pNext = nullptr;
			continue;
		}
	}

	// If only integrated GPU exists, use it by default.
	if (integratedDevice.has_value())
	{
		*physicalDeviceOut = integratedDevice.value();
		*physicalDeviceProperties2Out = integratedDeviceProperties2;
		*physicalDeviceIDPropertiesOut = integratedDeviceIDProperties;
		return;
	}

	// Fallback to the first device.
	*physicalDeviceOut = physicalDevices[0];
	pGetPhysicalDeviceProperties2(*physicalDeviceOut, physicalDeviceProperties2Out);
}

NAMESPACE_RENDERCORE_END
