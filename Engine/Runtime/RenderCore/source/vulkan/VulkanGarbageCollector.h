//
//  VulkanGarbageCollector.h
//  rendercore
//
//

#ifndef GNX_ENGINE_VULKAN_GARBAGE_COLLECTOR_INCLUDE_H
#define GNX_ENGINE_VULKAN_GARBAGE_COLLECTOR_INCLUDE_H

#include <vector>
#include <deque>
#include <memory>
#include "VulkanContext.h"
#include "Runtime/BaseLib/include/MutexLock.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * Vulkan 资源垃圾收集器
 *
 * 负责延迟销毁 Vulkan 资源，确保在 GPU 不再使用时才销毁资源。
 * 使用帧计数器追踪 GPU 进度，在特定帧后销毁资源。
 *
 * 注意：只有 GPU 实际使用的资源需要延迟释放，描述性对象可以立即销毁。
 */
class VulkanGarbageCollector
{
public:
    explicit VulkanGarbageCollector(VulkanContextPtr context);
    ~VulkanGarbageCollector();

    /**
     * 标记当前帧完成
     * 调用此函数来推进帧计数器，用于确定资源何时可以安全销毁
     */
    void AdvanceFrame();

    /**
     * 清理已过期的资源
     * 应该在每帧结束时调用
     */
    void Cleanup();

    /**
     * 立即清理所有资源
     * 用于关闭应用时，确保所有资源都被销毁
     */
    void ForceCleanupAll();

    /**
     * 添加要延迟销毁的 Buffer
     * @param buffer Vulkan buffer 句柄
     * @param allocation VMA 分配对象
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueueBufferDestruction(VkBuffer buffer, VmaAllocation allocation, uint32_t framesToDelay = 2);

    /**
     * 添加要延迟销毁的 Image
     * @param image Vulkan image 句柄
     * @param allocation VMA 分配对象
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueueImageDestruction(VkImage image, VmaAllocation allocation, uint32_t framesToDelay = 2);

    /**
     * 添加要延迟销毁的 ImageView
     * @param imageView Vulkan image view 句柄
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueueImageViewDestruction(VkImageView imageView, uint32_t framesToDelay = 2);

    /**
     * 添加要延迟销毁的 Sampler
     * @param sampler Vulkan sampler 句柄
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueueSamplerDestruction(VkSampler sampler, uint32_t framesToDelay = 2);

    /**
     * 添加要延迟销毁的 Framebuffer
     * @param framebuffer Vulkan framebuffer 句柄
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueueFramebufferDestruction(VkFramebuffer framebuffer, uint32_t framesToDelay = 2);

    /**
     * 添加要延迟销毁的 Pipeline
     * @param pipeline Vulkan pipeline 句柄（图形或计算）
     * @param framesToDelay 延迟多少帧后销毁（默认为 2 帧）
     */
    void QueuePipelineDestruction(VkPipeline pipeline, uint32_t framesToDelay = 2);

    /**
     * 获取当前帧编号
     */
    uint64_t GetCurrentFrame() const { return mCurrentFrame; }

    /**
     * 获取待销毁的资源数量（调试用）
     */
    size_t GetPendingResourceCount() const;

private:
    // 待销毁的资源结构
    struct PendingBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        uint64_t safeToDestroyFrame;
    };

    struct PendingImage
    {
        VkImage image;
        VmaAllocation allocation;
        uint64_t safeToDestroyFrame;
    };

    struct PendingImageView
    {
        VkImageView imageView;
        uint64_t safeToDestroyFrame;
    };

    struct PendingSampler
    {
        VkSampler sampler;
        uint64_t safeToDestroyFrame;
    };

    struct PendingFramebuffer
    {
        VkFramebuffer framebuffer;
        uint64_t safeToDestroyFrame;
    };

    struct PendingPipeline
    {
        VkPipeline pipeline;
        uint64_t safeToDestroyFrame;
    };

    // 清理特定类型的资源
    void CleanupBuffers();
    void CleanupImages();
    void CleanupImageViews();
    void CleanupSamplers();
    void CleanupFramebuffers();
    void CleanupPipelines();

    VulkanContextPtr mContext;
    uint64_t mCurrentFrame = 0;

    // 待销毁的资源队列
    std::deque<PendingBuffer> mPendingBuffers;
    std::deque<PendingImage> mPendingImages;
    std::deque<PendingImageView> mPendingImageViews;
    std::deque<PendingSampler> mPendingSamplers;
    std::deque<PendingFramebuffer> mPendingFramebuffers;
    std::deque<PendingPipeline> mPendingPipelines;

    mutable baselib::MutexLock mMutex;
};

using VulkanGarbageCollectorPtr = std::shared_ptr<VulkanGarbageCollector>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_GARBAGE_COLLECTOR_INCLUDE_H */
