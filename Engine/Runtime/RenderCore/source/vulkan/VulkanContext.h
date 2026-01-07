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
#include "Runtime/BaseLib/include/ThreadLocal.h"
#include "VKUtil.h"

NAMESPACE_RENDERCORE_BEGIN

// 前向声明
class VulkanGarbageCollector;

struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties;         // 设备属性
    VkPhysicalDeviceFeatures physicalDeviceFeatures;             // vulkan1.0的设备特性
    VkPhysicalDeviceMemoryProperties memoryProperties;           //内存属性

    VkPhysicalDeviceFeatures2 features2 = {};
    VkPhysicalDeviceVulkan11Features features_11 = {};
    VkPhysicalDeviceVulkan12Features features_12 = {};
    
    baselib::ThreadLocal commandPoolTls;
    baselib::ThreadLocal transferCommandPoolTls;
    
    VkCommandPool GetCommandPool();
    VkCommandPool GetTransferCommandPool();
    
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties;

    uint32_t graphicsQueueFamilyIndex;
    uint32_t graphicsQueueCount;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    uint32_t transferQueueFamilyIndex = 0;
    baselib::MutexLock transferQueuesLock;
    std::vector<VkQueue> availableTransferQueues;    // 所有可用的传输队列 

	uint32_t computeQueueFamilyIndex;
	std::vector<VkQueue> availableComputeQueues;    // 所有可用的计算队列 

    VkSurfaceKHR surfaceKhr = VK_NULL_HANDLE;     //surface
    VmaAllocator vmaAllocator;    //内存分配器
    VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
    
    //描述符集合的池子，分为计算和图形
    VkDescriptorPool graphicsDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool computeDescriptorPool = VK_NULL_HANDLE;
    
    std::vector<const char*> extensionNames;   //设备支持的扩展列表名称
    VulkanExtension vulkanExtension;

#ifdef NDEBUG
	bool enableValidationLayers = false;
#else
	bool enableValidationLayers = true;
#endif
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

    VulkanFencePool fencePool;
    UpLoadThreadPool upLoadPool;
    std::shared_ptr<VulkanGarbageCollector> garbageCollector;  // 资源垃圾收集器
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

// 创建图形描述符的pool
void CreateGraphicsDescriptorPool(VulkanContext& context);

// 创建计算描述符的pool
void CreateComputeDescriptorPool(VulkanContext& context);

// 创建垃圾收集器
void CreateGarbageCollector(VulkanContext& context);

// 清理垃圾收集器中的资源
void CleanupGarbageCollector(VulkanContext& context);

// 安全销毁资源（使用垃圾收集器或直接销毁）
// 注意：只有 GPU 实际使用的资源需要延迟释放
void SafeDestroyBuffer(VulkanContext& context, VkBuffer buffer, VmaAllocation allocation);
void SafeDestroyImage(VulkanContext& context, VkImage image, VmaAllocation allocation);
void SafeDestroyImageView(VulkanContext& context, VkImageView imageView);
void SafeDestroySampler(VulkanContext& context, VkSampler sampler);
void SafeDestroyFramebuffer(VulkanContext& context, VkFramebuffer framebuffer);
void SafeDestroyPipeline(VulkanContext& context, VkPipeline pipeline);

// 创建描述符集
VkDescriptorSet AllocDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descLayout);

void ChoosePhysicalDevice(
	const std::vector<VkPhysicalDevice>& physicalDevices,
	uint32_t preferredVendorID,
	uint32_t preferredDeviceID,
	const uint8_t* preferredDeviceUUID,
	const uint8_t* preferredDriverUUID,
	VkDriverId preferredDriverID,
	VkPhysicalDevice* physicalDeviceOut,
	VkPhysicalDeviceProperties2* physicalDeviceProperties2Out,
	VkPhysicalDeviceIDProperties* physicalDeviceIDPropertiesOut,
	VkPhysicalDeviceDriverProperties* physicalDeviceDriverPropertiesOut);

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
