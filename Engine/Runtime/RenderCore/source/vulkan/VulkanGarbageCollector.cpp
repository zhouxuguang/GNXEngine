//
//  VulkanGarbageCollector.cpp
//  rendercore
//
//

#include "VulkanGarbageCollector.h"
#include "VulkanContext.h"
#include "VKUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VulkanGarbageCollector::VulkanGarbageCollector(VulkanContextPtr context)
    : mContext(context)
    , mCurrentFrame(0)
{
}

VulkanGarbageCollector::~VulkanGarbageCollector()
{
    ForceCleanupAll();
}

void VulkanGarbageCollector::AdvanceFrame()
{
    baselib::AutoLock lock(mMutex);
    ++mCurrentFrame;
}

void VulkanGarbageCollector::Cleanup()
{
    baselib::AutoLock lock(mMutex);

    CleanupBuffers();
    CleanupImages();
    CleanupImageViews();
    CleanupSamplers();
    CleanupFramebuffers();
    CleanupPipelines();
}

void VulkanGarbageCollector::ForceCleanupAll()
{
    baselib::AutoLock lock(mMutex);

    // 强制清理所有资源，忽略帧计数
    VkDevice device = mContext->device;
    VmaAllocator allocator = mContext->vmaAllocator;

    // 清理所有 Buffers
    for (const auto& pending : mPendingBuffers)
    {
        if (pending.buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator, pending.buffer, pending.allocation);
        }
    }
    mPendingBuffers.clear();

    // 清理所有 Images
    for (const auto& pending : mPendingImages)
    {
        if (pending.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(allocator, pending.image, pending.allocation);
        }
    }
    mPendingImages.clear();

    // 清理所有 ImageViews
    for (const auto& pending : mPendingImageViews)
    {
        if (pending.imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, pending.imageView, nullptr);
        }
    }
    mPendingImageViews.clear();

    // 清理所有 Samplers
    for (const auto& pending : mPendingSamplers)
    {
        if (pending.sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, pending.sampler, nullptr);
        }
    }
    mPendingSamplers.clear();

    // 清理所有 Framebuffers
    for (const auto& pending : mPendingFramebuffers)
    {
        if (pending.framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, pending.framebuffer, nullptr);
        }
    }
    mPendingFramebuffers.clear();

    // 清理所有 Pipelines
    for (const auto& pending : mPendingPipelines)
    {
        if (pending.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, pending.pipeline, nullptr);
        }
    }
    mPendingPipelines.clear();
}

void VulkanGarbageCollector::QueueBufferDestruction(VkBuffer buffer, VmaAllocation allocation, uint32_t framesToDelay)
{
    if (buffer == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingBuffer pending;
    pending.buffer = buffer;
    pending.allocation = allocation;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingBuffers.push_back(pending);
}

void VulkanGarbageCollector::QueueImageDestruction(VkImage image, VmaAllocation allocation, uint32_t framesToDelay)
{
    if (image == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingImage pending;
    pending.image = image;
    pending.allocation = allocation;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingImages.push_back(pending);
}

void VulkanGarbageCollector::QueueImageViewDestruction(VkImageView imageView, uint32_t framesToDelay)
{
    if (imageView == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingImageView pending;
    pending.imageView = imageView;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingImageViews.push_back(pending);
}

void VulkanGarbageCollector::QueueSamplerDestruction(VkSampler sampler, uint32_t framesToDelay)
{
    if (sampler == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingSampler pending;
    pending.sampler = sampler;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingSamplers.push_back(pending);
}

void VulkanGarbageCollector::QueueFramebufferDestruction(VkFramebuffer framebuffer, uint32_t framesToDelay)
{
    if (framebuffer == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingFramebuffer pending;
    pending.framebuffer = framebuffer;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingFramebuffers.push_back(pending);
}

void VulkanGarbageCollector::QueuePipelineDestruction(VkPipeline pipeline, uint32_t framesToDelay)
{
    if (pipeline == VK_NULL_HANDLE)
    {
        return;
    }

    baselib::AutoLock lock(mMutex);

    PendingPipeline pending;
    pending.pipeline = pipeline;
    pending.safeToDestroyFrame = mCurrentFrame + framesToDelay;

    mPendingPipelines.push_back(pending);
}

size_t VulkanGarbageCollector::GetPendingResourceCount() const
{
    baselib::AutoLock lock(mMutex);

    return mPendingBuffers.size() +
           mPendingImages.size() +
           mPendingImageViews.size() +
           mPendingSamplers.size() +
           mPendingFramebuffers.size() +
           mPendingPipelines.size();
}

void VulkanGarbageCollector::CleanupBuffers()
{
    if (mPendingBuffers.empty())
    {
        return;
    }

    VkDevice device = mContext->device;
    VmaAllocator allocator = mContext->vmaAllocator;

    // 从队列前面清理已过期的资源
    while (!mPendingBuffers.empty())
    {
        const auto& pending = mPendingBuffers.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            // 还有资源未到期，停止清理
            break;
        }

        // 销毁 buffer
        if (pending.buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator, pending.buffer, pending.allocation);
        }

        mPendingBuffers.pop_front();
    }
}

void VulkanGarbageCollector::CleanupImages()
{
    if (mPendingImages.empty())
    {
        return;
    }

    VmaAllocator allocator = mContext->vmaAllocator;

    while (!mPendingImages.empty())
    {
        const auto& pending = mPendingImages.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            break;
        }

        if (pending.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(allocator, pending.image, pending.allocation);
        }

        mPendingImages.pop_front();
    }
}

void VulkanGarbageCollector::CleanupImageViews()
{
    if (mPendingImageViews.empty())
    {
        return;
    }

    VkDevice device = mContext->device;

    while (!mPendingImageViews.empty())
    {
        const auto& pending = mPendingImageViews.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            break;
        }

        if (pending.imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, pending.imageView, nullptr);
        }

        mPendingImageViews.pop_front();
    }
}

void VulkanGarbageCollector::CleanupSamplers()
{
    if (mPendingSamplers.empty())
    {
        return;
    }

    VkDevice device = mContext->device;

    while (!mPendingSamplers.empty())
    {
        const auto& pending = mPendingSamplers.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            break;
        }

        if (pending.sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, pending.sampler, nullptr);
        }

        mPendingSamplers.pop_front();
    }
}

void VulkanGarbageCollector::CleanupFramebuffers()
{
    if (mPendingFramebuffers.empty())
    {
        return;
    }

    VkDevice device = mContext->device;

    while (!mPendingFramebuffers.empty())
    {
        const auto& pending = mPendingFramebuffers.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            break;
        }

        if (pending.framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, pending.framebuffer, nullptr);
        }

        mPendingFramebuffers.pop_front();
    }
}

void VulkanGarbageCollector::CleanupPipelines()
{
    if (mPendingPipelines.empty())
    {
        return;
    }

    VkDevice device = mContext->device;

    while (!mPendingPipelines.empty())
    {
        const auto& pending = mPendingPipelines.front();

        if (pending.safeToDestroyFrame > mCurrentFrame)
        {
            break;
        }

        if (pending.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, pending.pipeline, nullptr);
        }

        mPendingPipelines.pop_front();
    }
}

NAMESPACE_RENDERCORE_END
