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

#include <mutex>

NAMESPACE_RENDERCORE_BEGIN

// 前向声明
class VulkanGarbageCollector;
struct VulkanContext;

struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties;         // 设备属性
    VkPhysicalDeviceFeatures physicalDeviceFeatures;             // vulkan1.0的设备特性
    VkPhysicalDeviceMemoryProperties memoryProperties;           //内存属性
    
    baselib::ThreadLocal commandPoolTls;
    baselib::ThreadLocal transferCommandPoolTls;
    baselib::ThreadLocal computeCommandPoolTls;

    VkCommandPool GetCommandPool();
    VkCommandPool GetTransferCommandPool();
    VkCommandPool GetComputeCommandPool();
    
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties;

    uint32_t graphicsQueueFamilyIndex;
    uint32_t graphicsQueueCount;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    uint32_t transferQueueFamilyIndex = 0;
    baselib::MutexLock transferQueuesLock;
    std::vector<VkQueue> availableTransferQueues;    // 所有可用的传输队列 

    // 串行化所有 VkQueue 的宿主访问，见 VulkanQueueAccess 的说明。
    // 必须可重入：Resize/OnWindowRestored 等路径内部会再次进入队列调用。
    std::recursive_mutex queueAccessLock;

	uint32_t computeQueueFamilyIndex;
	std::vector<VkQueue> availableComputeQueues;    // 所有可用的计算队列 

    VkSurfaceKHR surfaceKhr = VK_NULL_HANDLE;     //surface
    VmaAllocator vmaAllocator;    //内存分配器
    VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
    
    //描述符集合的池子，分为计算和图形
    VkDescriptorPool graphicsDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool computeDescriptorPool = VK_NULL_HANDLE;
    
    VulkanExtension vulkanExtension;
    DeviceExtFeature deviceExtFeatures;    // 设备扩展特性
    DeviceExtProperties deviceExtProperties;    // 设备扩展属性

    std::vector<const char*> deviceEnableExtensions;   //设备启用的扩展列表名称

#ifdef NDEBUG
	bool enableValidationLayers = false;
#else
	bool enableValidationLayers = true;
#endif
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

    // Pipeline Cache for caching compiled pipeline data
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

    VulkanFencePool fencePool;
    UpLoadThreadPool upLoadPool;
    std::shared_ptr<VulkanGarbageCollector> garbageCollector;  // 资源垃圾收集器

    // 异步计算同步信号量
    VkSemaphore asyncComputeSemaphore = VK_NULL_HANDLE;  // 图形和计算队列间的同步信号量

    // 时间线信号量（GPU 进度追踪，用于垃圾收集）
    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    uint64_t GetTimelineValue() const;

    void CollectDeviceExtension();
};

using VulkanContextPtr = std::shared_ptr<VulkanContext>;

/**
 * VkQueue 宿主访问串行化锁（RAII）。

 * Vulkan 规定对同一个 VkQueue 的宿主访问（vkQueueSubmit / vkQueueWaitIdle /
 * vkQueuePresentKHR）必须外部同步。引擎中渲染线程（提交帧、present、swapchain 重建）
 * 与资源上传线程（UpLoadThreadPool 提交 staging→image 拷贝、同步上传路径）会并发访问
 * 同一队列；当传输队列族与图形队列族相同时它们就是同一个 VkQueue。
 * 未加锁的并发提交属于未定义行为，驱动侧最常见的表现就是 VK_ERROR_DEVICE_LOST。

 * 所有涉及 VkQueue 的调用点都必须用本类串行化。
 */
class VulkanQueueAccess
{
public:
    explicit VulkanQueueAccess(VulkanContext& context) : mContext(context)
    {
        mContext.queueAccessLock.lock();
    }

    ~VulkanQueueAccess()
    {
        mContext.queueAccessLock.unlock();
    }

    VulkanQueueAccess(const VulkanQueueAccess&) = delete;
    VulkanQueueAccess& operator=(const VulkanQueueAccess&) = delete;

private:
    VulkanContext& mContext;
};

// 根据API版本创建实例
bool CreateInstance(VulkanContext& context, uint32_t apiVersion);

// 选择合适的设备
bool SelectPhysicalDevice(VulkanContext& context);

// 创建虚拟设备，即VKDevice对象
bool CreateVirtualDevice(VulkanContext& context);

// 创建VMA的内存分配器
void CreateVMA(VulkanContext& context);

// 创建Surface对象
bool CreateSurfaceKHR(VulkanContext& context, const NativeWindow& nativeWindow);

bool DestroySurfaceKHR(VulkanContext& context);

// 创建图形描述符的pool
void CreateGraphicsDescriptorPool(VulkanContext& context);

// 创建计算描述符的pool
void CreateComputeDescriptorPool(VulkanContext& context);

// 创建垃圾收集器
void CreateGarbageCollector(VulkanContext& context);

// 初始化 Pipeline Cache（在 VkDevice 创建后调用）
bool InitializePipelineCache(VulkanContext& context);

// 仅保存 Pipeline Cache 到磁盘（不销毁 VkPipelineCache，可多次调用）
void SavePipelineCache(VulkanContext& context);

// 保存 Pipeline Cache 到磁盘并销毁
void SaveAndDestroyPipelineCache(VulkanContext& context);

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
    VulkanImageView(const VulkanContextPtr& context, VkImageView imageView)
        : context(context), imageView(imageView)
    {
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
        if (context && context->device != VK_NULL_HANDLE && imageView != VK_NULL_HANDLE)
        {
            // Image views referenced by a descriptor remain in use until the
            // submitted frame has completed.  Destroying the view immediately
            // while its image is deferred leaves an invalid descriptor in an
            // in-flight command buffer (tile eviction during zoom hits this
            // path frequently), which can result in VK_ERROR_DEVICE_LOST.
            SafeDestroyImageView(*context, imageView);
            imageView = VK_NULL_HANDLE;
        }
    }
private:
    VulkanContextPtr context;
    VkImageView imageView = VK_NULL_HANDLE;
};

using VulkanImageViewPtr = std::shared_ptr<VulkanImageView>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_CONTEXT_INCLUDE_GFDGJ */
