//
//  VKTextureBase.cpp
//  rendercore
//
//  Created by zhouxuguang on 2025/9/20.
//

#include "VKTextureBase.h"
#include "TextureFormat.h"
#include "VulkanBufferUtil.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <atomic>
#include <mutex>

NAMESPACE_RENDERCORE_BEGIN

namespace
{
class VulkanTextureUpload final : public TextureUpload
{
public:
    VulkanTextureUpload(VulkanContextPtr context, VKRCTexture2DPtr texture,
                        VkBuffer staging, VmaAllocation allocation,
                        VkCommandPool transferPool,
                        VkSemaphore semaphore, VkFence fence)
        : mContext(std::move(context)), mTexture(std::move(texture)),
          mStaging(staging), mAllocation(allocation),
          mTransferPool(transferPool),
          mSemaphore(semaphore), mFence(fence) {}

    ~VulkanTextureUpload() override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // The ticket also retains the texture. Eviction may destroy the ticket
        // before completion, so finish the GPU work before releasing either.
        if (mFence != VK_NULL_HANDLE)
            vkWaitForFences(mContext->device, 1, &mFence, VK_TRUE, UINT64_MAX);
        ReleaseResources();
    }

    TextureUploadStatus GetStatus() const override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mStatus != TextureUploadStatus::Pending)
            return mStatus;
        const VkResult result = vkGetFenceStatus(mContext->device, mFence);
        if (result == VK_SUCCESS)
        {
            mStatus = TextureUploadStatus::Complete;
            ReleaseResources();
            static std::atomic<bool> loggedRelease{false};
            if (!loggedRelease.exchange(true))
                LOG_INFO("[Vulkan] Completed texture upload staging and sync objects released");
        }
        else if (result != VK_NOT_READY)
        {
            LOG_ERROR("Vulkan texture upload failed: %d", static_cast<int>(result));
            mStatus = TextureUploadStatus::Failed;
        }
        return mStatus;
    }

private:
    void ReleaseResources() const
    {
        if (!mContext) return;
        const VulkanContextPtr context = mContext;
        if (mFence != VK_NULL_HANDLE)
            vkDestroyFence(context->device, mFence, nullptr);
        if (mSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(context->device, mSemaphore, nullptr);
        // This pool belongs solely to this upload; another thread may destroy
        // the ticket without racing the uploader's thread-local pool.
        if (mTransferPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(context->device, mTransferPool, nullptr);
        if (mStaging != VK_NULL_HANDLE)
            vmaDestroyBuffer(context->vmaAllocator, mStaging, mAllocation);
        mFence = VK_NULL_HANDLE;
        mSemaphore = VK_NULL_HANDLE;
        mTransferPool = VK_NULL_HANDLE;
        mStaging = VK_NULL_HANDLE;
        mAllocation = VK_NULL_HANDLE;
        mTexture.reset();
        mContext.reset();
    }

    mutable std::mutex mMutex;
    mutable TextureUploadStatus mStatus = TextureUploadStatus::Pending;
    mutable VulkanContextPtr mContext;
    mutable VKRCTexture2DPtr mTexture;
    mutable VkBuffer mStaging;
    mutable VmaAllocation mAllocation;
    mutable VkCommandPool mTransferPool;
    mutable VkSemaphore mSemaphore;
    mutable VkFence mFence;
};
}

bool VKTextureBase::GetASTCDecodeMode(VkImageViewASTCDecodeModeEXT& astcDecodeMode) const
{
    // 只有扩展已启用且纹理是 ASTC LDR 格式时才需要/允许指定解码模式
    if (!mContext->vulkanExtension.enableAstcDecodeMode)
    {
        return false;
    }
    if (!VulkanBufferUtil::IsASTCLDRFormat(mFormat))
    {
        return false;
    }

    astcDecodeMode = {};
    astcDecodeMode.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
    astcDecodeMode.pNext = nullptr;
    // 按纹理格式的 sRGB 属性选择解码模式：
    //   - SRGB 格式 → SRGB 解码（默认行为，显式指定）
    //   - UNORM 格式 → UNORM（线性）解码
    astcDecodeMode.decodeMode = VulkanBufferUtil::IsSRGBFormat(mFormat)
        ? VK_FORMAT_R8G8B8A8_SRGB
        : VK_FORMAT_R8G8B8A8_UNORM;
    return true;
}

void VKTextureBase::CreateImageViews(const VkImageCreateInfo& imageCreateInfo)
{
    //创建图像视图
    VkImageAspectFlags imageAspectFlags = VulkanBufferUtil::GetImageAspectFlags(mFormat);

    // ASTC LDR 解码模式（VK_EXT_astc_decode_mode），非 ASTC LDR 或扩展不可用时为 nullptr
    VkImageViewASTCDecodeModeEXT astcDecodeMode = {};
    const VkImageViewASTCDecodeModeEXT* astcDecodeModePtr =
        GetASTCDecodeMode(astcDecodeMode) ? &astcDecodeMode : nullptr;

    // Shader view 覆盖全 mip，attachment view 保持单 mip。
    const bool usedAsAttachment =
        (imageCreateInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0 ||
        (imageCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
    const uint32_t shaderViewLevelCount = usedAsAttachment ? 1 : imageCreateInfo.mipLevels;

    VkImageView imageView = VK_NULL_HANDLE;
    if (GetTextureType() == TextureType_2D)
    {
        imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, mFormat,
            nullptr, imageAspectFlags, shaderViewLevelCount, astcDecodeModePtr);
    }

    else if (GetTextureType() == TextureType_3D)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = mImage;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		viewCreateInfo.format = mFormat;
		viewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;
		viewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        viewCreateInfo.pNext = astcDecodeModePtr;
        vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);
    }

    else if (GetTextureType() == TextureType_CUBE)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        // Cube map view type
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCreateInfo.format = mFormat;
        viewCreateInfo.subresourceRange = { imageAspectFlags, 0, 1, 0, 1 };
        // 6 array layers (faces)
        viewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        viewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        viewCreateInfo.image = mImage;
        viewCreateInfo.pNext = astcDecodeModePtr;
        vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);
    }

    else if (GetTextureType() == TextureType_2D_ARRAY)
    {
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = mImage;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		viewCreateInfo.format = mFormat;
		viewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = imageCreateInfo.arrayLayers;
		viewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        viewCreateInfo.pNext = astcDecodeModePtr;
		vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);
    }
     
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext, imageView);
    
    // 针对rt，还需要创建特殊的view
    if ((imageCreateInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
        (imageCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_3D;

        uint32_t viewCount = 0;
        if (GetTextureType() == TextureType_2D_ARRAY)
        {
            viewCount = imageCreateInfo.arrayLayers;
            viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        else if (GetTextureType() == TextureType_3D)
        {
            viewCount = mDepth;
            viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;   // 这里创建view的时候当做2d array
        }

        if (viewCount > 1)
        {
            for (uint32_t nLayer = 0; nLayer < viewCount; nLayer ++)
            {
				VkImageViewCreateInfo viewCreateInfo = {};
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.image = mImage;
				viewCreateInfo.viewType = viewType;
				viewCreateInfo.format = mFormat;
				viewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
                viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = nLayer;
				viewCreateInfo.subresourceRange.layerCount = 1;
                viewCreateInfo.pNext = astcDecodeModePtr;

                VkImageView imageView = VK_NULL_HANDLE;
				vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);

                mRenderTargetViews.push_back(std::make_shared<VulkanImageView>(mContext, imageView));
            }
        }
    }
}

VKTextureBase::VKTextureBase(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo) : 
    RCTexture(TextureType_Unkown),
    mContext(context)
{
    // 判断该格式是否支持HostImageCopy
    VkFormatProperties3 formatProperties3 = {};
    formatProperties3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;

    // Properties3 need to be chained into Properties2
    VkFormatProperties2 formatProperties2 = {};
    formatProperties2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
    formatProperties2.pNext = &formatProperties3;

    // 获得格式的属性
    vkGetPhysicalDeviceFormatProperties2(mContext->physicalDevice, imageCreateInfo.format, &formatProperties2);

    mSupportHostImageCopy = mContext->vulkanExtension.enableHostImageCopy;
    if ((formatProperties3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) == 0)
    {
        mSupportHostImageCopy = false;
    }

    // 渲染目标禁用 host image copy：带 VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT 的图像
    // 会被驱动关闭渲染目标压缩（NVIDIA DCC），4K 下整帧 5ms → 23ms。
    // 此处 usage 来自 ConvertTextureUsage，附件位即代表「声明为渲染目标」。
    const bool isDeclaredRenderTarget =
        (imageCreateInfo.usage &
         (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
    if (isDeclaredRenderTarget)
    {
        mSupportHostImageCopy = false;
    }
    
    const VkFormatFeatureFlags2 formatFeatures = formatProperties3.optimalTilingFeatures;
    VkImageCreateInfo imageCreateInfoCopy = imageCreateInfo;

    if ((formatFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT) != 0)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if ((formatFeatures & VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT) != 0)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if ((formatFeatures & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT) != 0 &&
        !VulkanBufferUtil::IsSRGBFormat(imageCreateInfo.format))
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (VulkanBufferUtil::IsDepthStencilFormat(imageCreateInfo.format))
    {
    if ((formatFeatures & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    }
    else if ((formatFeatures & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT) != 0)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (mSupportHostImageCopy)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
    }
    
    //创建图像
    VkResult result = VulkanBufferUtil::CreateImageGeneral(mContext->vmaAllocator, imageCreateInfoCopy, mImage, mAllocation);
    assert(result == VK_SUCCESS);
    
    mFormat = imageCreateInfoCopy.format;
    mWidth = imageCreateInfoCopy.extent.width;
    mHeight = imageCreateInfoCopy.extent.height;
    mDepth = imageCreateInfoCopy.extent.depth;
    mMipLevels = imageCreateInfoCopy.mipLevels;
    mLayerCount = imageCreateInfoCopy.arrayLayers;

    mSubresourceLayouts.assign(
        static_cast<size_t>(mMipLevels) * (mLayerCount > 0 ? mLayerCount : 1),
        VK_IMAGE_LAYOUT_UNDEFINED);
    mImageUsage = imageCreateInfoCopy.usage;

    assert(mImage != VK_NULL_HANDLE);
}

VKTextureBase::~VKTextureBase()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }

    if (mImage != VK_NULL_HANDLE)
    {
        // 使用垃圾收集器延迟销毁
        SafeDestroyImage(*mContext, mImage, mAllocation);
        mImage = VK_NULL_HANDLE;
    }

    if (mVulkanImageViewPtr)
    {
        mVulkanImageViewPtr = nullptr;
    }
    
    // 清理 mip level 视图缓存
    mMipLevelViews.clear();
}

VulkanImageViewPtr VKTextureBase::GetMipLevelImageView(uint32_t mipLevel)
{
    // 检查 mipLevel 是否有效
    if (mipLevel >= mMipLevels)
    {
        assert(false && "Invalid mip level");
        return nullptr;
    }
    
    // 检查缓存
    auto it = mMipLevelViews.find(mipLevel);
    if (it != mMipLevelViews.end())
    {
        return it->second;
    }
    
    // 创建新的单 mip level 视图
    VkImageAspectFlags imageAspectFlags = VulkanBufferUtil::GetImageAspectFlags(mFormat);

    // ASTC LDR 解码模式（VK_EXT_astc_decode_mode）
    VkImageViewASTCDecodeModeEXT astcDecodeMode = {};
    const VkImageViewASTCDecodeModeEXT* astcDecodeModePtr =
        GetASTCDecodeMode(astcDecodeMode) ? &astcDecodeMode : nullptr;

    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = mImage;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = mFormat;
    viewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel = mipLevel;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = mLayerCount;
    viewCreateInfo.pNext = astcDecodeModePtr;
    
    VkImageView imageView = VK_NULL_HANDLE;
    VkResult result = vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);
    assert(result == VK_SUCCESS);
    
    auto viewPtr = std::make_shared<VulkanImageView>(mContext, imageView);
    mMipLevelViews[mipLevel] = viewPtr;
    
    return viewPtr;
}

void VKTextureBase::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    if (!pixelBytes)
    {
        return;
    }

    if (mSupportHostImageCopy)
    {
        // 只检查驱动返回的有效 layout。
        const std::vector<VkImageLayout>& dstLayouts =
            mContext->deviceExtProperties.hostImageCopyDstLayoutsStorage;
        const uint32_t dstLayoutCount = std::min<uint32_t>(
            mContext->deviceExtProperties.hostImageCopyProperties.copyDstLayoutCount,
            static_cast<uint32_t>(dstLayouts.size()));

        VkImageLayout hostCopyDstLayout = VK_IMAGE_LAYOUT_GENERAL;
        bool foundSupportedLayout = false;
        for (uint32_t i = 0; i < dstLayoutCount; ++i)
        {
            const VkImageLayout layout = dstLayouts[i];
            if (layout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                continue;
            }
            if (!foundSupportedLayout)
            {
                hostCopyDstLayout = layout;
                foundSupportedLayout = true;
            }
            if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                hostCopyDstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;
            }
            if (layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                hostCopyDstLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
        }

        // 图像layout转换
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = level;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = slice;
        subresourceRange.layerCount = 1;

        // 增量上传必须使用 subresource 的实际 oldLayout。
        const VkImageLayout currentMipLayout = GetSubresourceLayout(level, slice);

        VkHostImageLayoutTransitionInfoEXT hostImageLayoutTransitionInfo = {};
        hostImageLayoutTransitionInfo.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT;
        hostImageLayoutTransitionInfo.image = mImage;
        hostImageLayoutTransitionInfo.oldLayout = currentMipLayout;
        hostImageLayoutTransitionInfo.newLayout = hostCopyDstLayout;
        hostImageLayoutTransitionInfo.subresourceRange = subresourceRange;

        vkTransitionImageLayoutEXT(mContext->device, 1, &hostImageLayoutTransitionInfo);

        // Step 2: 使用Host端copy上传纹理数据，dstImageLayout必须与上面的newLayout一致
        VkMemoryToImageCopyEXT memoryToImageCopy = {};
        memoryToImageCopy.sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT;
        memoryToImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        memoryToImageCopy.imageSubresource.mipLevel = level;
        memoryToImageCopy.imageSubresource.baseArrayLayer = slice;
        memoryToImageCopy.imageSubresource.layerCount = 1;
        memoryToImageCopy.imageOffset.x = rect.offsetX;
        memoryToImageCopy.imageOffset.y = rect.offsetY;
        memoryToImageCopy.imageOffset.z = 0;      // 3d纹理这里如何处理
        memoryToImageCopy.imageExtent.width = rect.width;
        memoryToImageCopy.imageExtent.height = rect.height;
        memoryToImageCopy.imageExtent.depth = 1;
        memoryToImageCopy.pHostPointer = pixelBytes;

        VkCopyMemoryToImageInfoEXT copyMemoryInfo = {};
        copyMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
        copyMemoryInfo.dstImage = mImage;
        copyMemoryInfo.dstImageLayout = hostCopyDstLayout;
        copyMemoryInfo.regionCount = 1;
        copyMemoryInfo.pRegions = &memoryToImageCopy;

        vkCopyMemoryToImageEXT(mContext->device, &copyMemoryInfo);

        // 恢复到纹理用途允许的最终 layout。
        const VkImageLayout finalLayout =
            (mImageUsage & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) != 0
                ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                : VK_IMAGE_LAYOUT_GENERAL;

        if (hostCopyDstLayout != finalLayout)
        {
            VkCommandPool cmdPool = mContext->GetCommandPool();
            VkCommandBuffer cmdBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, cmdPool);
            VulkanBufferUtil::SetImageLayout(cmdBuffer, mImage,
                hostCopyDstLayout, finalLayout,
                subresourceRange);
            VulkanBufferUtil::EndSingleTimeCommand(*mContext, mContext->graphicsQueue, cmdPool, cmdBuffer);
        }

        SetSubresourceLayout(level, slice, finalLayout);
    }
    else
    {
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkDeviceSize size = bytesPerRow * rect.height;
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            stageBuffer, allocation, nullptr);

        void* data = nullptr;
        vmaMapMemory(mContext->vmaAllocator, allocation, &data);
        memcpy(data, pixelBytes, size);
        vmaUnmapMemory(mContext->vmaAllocator, allocation);

        UpLoadTaskPtr upLoadTask = std::make_shared<UpLoadTask>();
        upLoadTask->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        upLoadTask->subresourceRange.baseMipLevel = level;
        upLoadTask->subresourceRange.baseArrayLayer = slice;
        upLoadTask->subresourceRange.levelCount = 1;
        upLoadTask->subresourceRange.layerCount = 1;

        VkImage image = mImage;

        upLoadTask->allocation = allocation;
        upLoadTask->rect = rect;
        upLoadTask->mImage = image;
        upLoadTask->stageBuffer = stageBuffer;
        upLoadTask->mContext = mContext;
        upLoadTask->oldLayout = GetSubresourceLayout(level, slice);
        SetSubresourceLayout(level, slice, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        mContext->upLoadPool.Execute(upLoadTask);
    }
}

void VKTextureBase::ReplaceRegionSynchronous(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    if (!pixelBytes || rect.width == 0 || rect.height == 0)
    {
        return;
    }

    // 走 staging buffer + 一次性命令缓冲区：在调用线程上提交到图形队列并等待完成。
    // 与 ReplaceRegion 的异步路径（提交到传输队列、且与图形队列没有任何同步）不同，
    // 这里保证函数返回时：
    //   1) 纹素数据已经写进图像；
    //   2) 图像布局已经转换为 SHADER_READ_ONLY_OPTIMAL，
    // 所以调用方紧接着录制的绘制可以安全采样该纹理。
    const VkDeviceSize size = (VkDeviceSize)bytesPerRow * (VkDeviceSize)rect.height;

    VkBuffer stageBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stageBuffer, allocation, nullptr);

    if (stageBuffer == VK_NULL_HANDLE || allocation == VK_NULL_HANDLE)
    {
        LOG_ERROR("VKTextureBase::ReplaceRegionSynchronous: create staging buffer failed (size=%llu)",
                  (unsigned long long)size);
        return;
    }

    void* mapped = nullptr;
    if (vmaMapMemory(mContext->vmaAllocator, allocation, &mapped) == VK_SUCCESS && mapped != nullptr)
    {
        memcpy(mapped, pixelBytes, (size_t)size);
        vmaUnmapMemory(mContext->vmaAllocator, allocation);
    }
    else
    {
        LOG_ERROR("VKTextureBase::ReplaceRegionSynchronous: map staging buffer failed");
        vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
        return;
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = level;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = slice;
    subresourceRange.layerCount = 1;

    VkCommandPool cmdPool = mContext->GetCommandPool();
    VkCommandBuffer cmdBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, cmdPool);

    // 使用 subresource 的实际 oldLayout，不能假设是 UNDEFINED
    const VkImageLayout oldLayout = GetSubresourceLayout(level, slice);
    VulkanBufferUtil::SetImageLayout(cmdBuffer, mImage, oldLayout,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    VulkanBufferUtil::CopyBufferToImage(mContext->device, cmdBuffer, stageBuffer, mImage,
        rect.offsetX, rect.offsetY, rect.width, rect.height, level, slice);

    VulkanBufferUtil::SetImageLayout(cmdBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    // 阻塞等待本次拷贝完成（内部 vkQueueSubmit + vkWaitForFences）
    VulkanBufferUtil::EndSingleTimeCommand(*mContext, mContext->graphicsQueue, cmdPool, cmdBuffer);

    vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
    SetSubresourceLayout(level, slice, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

bool VKTextureBase::IsValid() const
{
    return mImage != VK_NULL_HANDLE && mVulkanImageViewPtr;
}

uint32_t VKTextureBase::GetWidth() const
{
    return mWidth;
}

uint32_t VKTextureBase::GetHeight() const
{
    return mHeight;
}

uint32_t VKTextureBase::GetDepth() const
{
    return mDepth;
}

uint32_t VKTextureBase::GetMipLevels() const
{
    return mMipLevels;
}

uint32_t VKTextureBase::GetLayerCount() const
{
    return mLayerCount;
}

void VKTextureBase::SetName(const char* name)
{
    SetObjectName(mContext->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)mImage, name);
}

#pragma mark VKRCTexture2D

VKRCTexture2D::VKRCTexture2D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo), RCTexture(TextureType_2D), mUploadContext(context)
{
}

VKRCTexture2D::~VKRCTexture2D()
{
}

void VKRCTexture2D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    VKTextureBase::ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
}

void VKRCTexture2D::ReplaceRegionSync(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    VKTextureBase::ReplaceRegionSynchronous(rect, level, 0, pixelBytes, bytesPerRow);
}

TextureUploadPtr VKRCTexture2D::ReplaceRegionAsync(const Rect2D& rect, uint32_t level,
                                                    const uint8_t* pixels, uint32_t bytesPerRow)
{
    auto failed = [] { return std::make_shared<FailedTextureUpload>(); };
    if (!pixels || !IsValid() || rect.width <= 0 || rect.height <= 0 ||
        level >= GetMipLevels() || mUploadContext->availableTransferQueues.empty() ||
        (GetVKUsage() & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
        return failed();

    // Updates to an already sampled subresource need additional ordering with
    // prior graphics reads. Preserve that case via the existing sync path.
    if (GetSubresourceLayout(level, 0) != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        ReplaceRegionSync(rect, level, pixels, bytesPerRow);
        return std::make_shared<CompletedTextureUpload>();
    }

    const TextureBlockInfo block = GetCompressedTextureBlockInfo(GetTextureFormat());
    const uint32_t blockWidth = block.bytesPerBlock ? block.blockWidth : 1u;
    const uint32_t blockHeight = block.bytesPerBlock ? block.blockHeight : 1u;
    const uint32_t bytesPerElement = block.bytesPerBlock
        ? block.bytesPerBlock : VulkanBufferUtil::GetFormatSize(GetVKFormat());
    if (!bytesPerElement || bytesPerRow % bytesPerElement != 0)
        return failed();
    const uint32_t rowLength = bytesPerRow / bytesPerElement * blockWidth;
    const uint32_t rows = (static_cast<uint32_t>(rect.height) + blockHeight - 1) / blockHeight;
    if (rowLength < static_cast<uint32_t>(rect.width)) return failed();
    const VkDeviceSize size = static_cast<VkDeviceSize>(bytesPerRow) * rows;
    VkBuffer staging = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mUploadContext->vmaAllocator, StorageModeShared,
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, staging, allocation, nullptr);
    if (!staging || !allocation) return failed();

    void* mapped = nullptr;
    if (vmaMapMemory(mUploadContext->vmaAllocator, allocation, &mapped) != VK_SUCCESS || !mapped)
    {
        vmaDestroyBuffer(mUploadContext->vmaAllocator, staging, allocation);
        return failed();
    }
    memcpy(mapped, pixels, static_cast<size_t>(size));
    vmaFlushAllocation(mUploadContext->vmaAllocator, allocation, 0, size);
    vmaUnmapMemory(mUploadContext->vmaAllocator, allocation);

    const VkDevice device = mUploadContext->device;
    VkCommandPool transferPool = VK_NULL_HANDLE;
    VkCommandBuffer transferCommand = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    auto cleanup = [&] {
        if (fence) vkDestroyFence(device, fence, nullptr);
        if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
        if (transferPool) vkDestroyCommandPool(device, transferPool, nullptr);
        vmaDestroyBuffer(mUploadContext->vmaAllocator, staging, allocation);
    };
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = mUploadContext->transferQueueFamilyIndex;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &transferPool) != VK_SUCCESS)
        { cleanup(); return failed(); }
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.commandPool = transferPool;
    if (vkAllocateCommandBuffers(device, &allocateInfo, &transferCommand) != VK_SUCCESS)
        { cleanup(); return failed(); }
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
        { cleanup(); return failed(); }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = level;
    range.levelCount = 1;
    range.layerCount = 1;
    const VkImage image = GetVKImage();
    if (vkBeginCommandBuffer(transferCommand, &beginInfo) != VK_SUCCESS)
        { cleanup(); return failed(); }
    VulkanBufferUtil::InsertImageMemoryBarrier(transferCommand, image, 0,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, range);
    VkBufferImageCopy copy{};
    copy.bufferRowLength = rowLength;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = level;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {rect.offsetX, rect.offsetY, 0};
    copy.imageExtent = {static_cast<uint32_t>(rect.width),
                        static_cast<uint32_t>(rect.height), 1};
    vkCmdCopyBufferToImage(transferCommand, staging, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    VulkanBufferUtil::InsertImageMemoryBarrier(transferCommand, image,
        VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, range);
    if (vkEndCommandBuffer(transferCommand) != VK_SUCCESS)
        { cleanup(); return failed(); }

    VkSubmitInfo transferSubmit{};
    transferSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transferSubmit.commandBufferCount = 1;
    transferSubmit.pCommandBuffers = &transferCommand;
    transferSubmit.signalSemaphoreCount = 1;
    transferSubmit.pSignalSemaphores = &semaphore;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo graphicsSubmit{};
    graphicsSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmit.waitSemaphoreCount = 1;
    graphicsSubmit.pWaitSemaphores = &semaphore;
    graphicsSubmit.pWaitDstStageMask = &waitStage;
    // An empty graphics submit waits for the transfer and signals our fence.
    // There is no need to allocate a graphics-family command buffer.
    {
        VulkanQueueAccess queueAccess(*mUploadContext);
        VkResult result = vkQueueSubmit(mUploadContext->availableTransferQueues[0],
                                        1, &transferSubmit, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) { cleanup(); return failed(); }
        result = vkQueueSubmit(mUploadContext->graphicsQueue, 1, &graphicsSubmit, fence);
        if (result != VK_SUCCESS)
        {
            vkQueueWaitIdle(mUploadContext->availableTransferQueues[0]);
            cleanup();
            return failed();
        }
    }
    SetSubresourceLayout(level, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    static bool loggedTransferFamily = false;
    if (!loggedTransferFamily)
    {
        LOG_INFO("[Vulkan] Asynchronous texture upload: transfer family %u, graphics family %u",
                 mUploadContext->transferQueueFamilyIndex,
                 mUploadContext->graphicsQueueFamilyIndex);
        loggedTransferFamily = true;
    }
    return std::make_shared<VulkanTextureUpload>(mUploadContext, shared_from_this(),
        staging, allocation, transferPool, semaphore, fence);
}

#pragma mark VKRCTexture3D

VKRCTexture3D::VKRCTexture3D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo), RCTexture(TextureType_3D)
{
}

VKRCTexture3D::~VKRCTexture3D()
{
}

void VKRCTexture3D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    VKTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark VKRCTextureCube

VKRCTextureCube::VKRCTextureCube(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo), RCTexture(TextureType_CUBE)
{
}

VKRCTextureCube::~VKRCTextureCube()
{
}

void VKRCTextureCube::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    VKTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark VKRCTexture2DArray

VKRCTexture2DArray::VKRCTexture2DArray(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo), RCTexture(TextureType_2D_ARRAY)
{
}

VKRCTexture2DArray::~VKRCTexture2DArray()
{
}

void VKRCTexture2DArray::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    VKTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

NAMESPACE_RENDERCORE_END
