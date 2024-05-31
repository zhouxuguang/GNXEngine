//
//  VulkanContext.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_VK_RENDER_CONTEXT_INCLUDE_GFDGJ
#define GNX_ENGINE_VK_RENDER_CONTEXT_INCLUDE_GFDGJ

#include "VKRenderDefine.h"
#include "RenderDevice.h"
#include "VulkanExtension.h"

NAMESPACE_RENDERCORE_BEGIN

struct VulkanContext 
{
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;
    uint32_t queueCount;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surfaceKhr = VK_NULL_HANDLE;     //surface
    
    bool debugMarkersSupported;     // 是否支持调试
    VkDebugReportCallbackEXT debugReportCallbackExt;   // debugreport
    
    VmaAllocator vmaAllocator;    //内存分配器
    VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
    
    //描述符集合的池子，分为计算和图形
    VkDescriptorPool computeDescriptorPool = VK_NULL_HANDLE;
    
    std::vector<const char*> extensionNames;   //设备支持的扩展列表名称
    VulkanExtension vulkanExtension;
};

using VulkanContextPtr = std::shared_ptr<VulkanContext>;

// 根据API版本创建实例
bool CreateInstance(VulkanContext& context, uint32_t apiVersion);

// 选择合适的设备
bool SelectPhysicalDevice(VulkanContext& context);

// 创建虚拟设备，即VKDevice对象
bool CreateVirtualDevice(VulkanContext& context);

// 创建VMA的内存分配器
void CreateVMA(VulkanContext& context);

// 创建Surface对象
bool CreateSurfaceKHR(VulkanContext& context, ViewHandle nativeWidow);

// 创建计算描述符的pool
void CreateComputeDescriptorPool(VulkanContext& context);

// 创建描述符集
VkDescriptorSet AllocDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descLayout);

//图像视图的包装
struct VulkanImageView
{
public:
    VulkanImageView(VkDevice device, VkImageView imageView)
    {
        this->device = device;
        this->imageView = imageView;
    }
    
    ~VulkanImageView()
    {
        Release();
    }
    
    VkImageView GetHandle() const
    {
        return imageView;
    }
    
    void Release()
    {
        if (device != VK_NULL_HANDLE && imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, imageView, nullptr);
            device = VK_NULL_HANDLE;
            imageView = VK_NULL_HANDLE;
        }
    }
private:
    VkDevice device = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
};

using VulkanImageViewPtr = std::shared_ptr<VulkanImageView>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_CONTEXT_INCLUDE_GFDGJ */
