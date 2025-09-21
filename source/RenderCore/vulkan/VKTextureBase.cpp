//
//  VKTextureBase.cpp
//  rendercore
//
//  Created by zhouxuguang on 2025/9/20.
//

#include "VKTextureBase.h"
#include "TextureFormat.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKTextureBase::VKTextureBase(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo) : mContext(context)
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
    
    // 修改相应的标记
    VkImageCreateInfo imageCreateInfoCopy = imageCreateInfo;
    
    if (mSupportHostImageCopy)
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
    }
    else
    {
        imageCreateInfoCopy.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    
    //创建图像
    VulkanBufferUtil::CreateImageGeneral(mContext->vmaAllocator, imageCreateInfoCopy, mImage, mAllocation);
    
    mFormat = imageCreateInfoCopy.format;
    mWidth = imageCreateInfoCopy.extent.width;
    mHeight = imageCreateInfoCopy.extent.height;
    mDepth = imageCreateInfoCopy.extent.depth;

    //创建图像视图
    VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (VulkanBufferUtil::IsDepthStencilFormat(mFormat))
    {
        imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    
    VkImageView imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, mFormat,
            nullptr, imageAspectFlags, 1);
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext->device, imageView);
    
    // 针对rt，还需要创建特殊的view
    if ((imageCreateInfoCopy.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
        (imageCreateInfoCopy.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        uint32_t viewCount = 0;
        if (GetTextureType() == TextureType_2D_ARRAY)
        {
            viewCount = imageCreateInfoCopy.arrayLayers;
        }
        if (GetTextureType() == TextureType_3D)
        {
            viewCount = mDepth;
        }

        if (viewCount > 1)
        {
            VkImageView imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, mFormat,
            nullptr, imageAspectFlags, 1);
        }
    }
}

VKTextureBase::~VKTextureBase()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }

    if (mImage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(mContext->vmaAllocator, mImage, mAllocation);
        mImage = VK_NULL_HANDLE;
    }

    if (mVulkanImageViewPtr)
    {
        mVulkanImageViewPtr = nullptr;
    }
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
        // 图像layout转换
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = level;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = slice;
        subresourceRange.layerCount = 1;

        // VK_EXT_host_image_copy also introduces a simplified way of doing the required image transition on the host
        // This no longer requires a dedicated command buffer to submit the barrier
        // We also no longer need multiple transitions, and only have to do one for the final layout
        VkHostImageLayoutTransitionInfoEXT hostImageLayoutTransitionInfo = {};
        hostImageLayoutTransitionInfo.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT;
        hostImageLayoutTransitionInfo.image = mImage;
        hostImageLayoutTransitionInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        hostImageLayoutTransitionInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        hostImageLayoutTransitionInfo.subresourceRange = subresourceRange;

        // Get the function pointers required host image copies
        PFN_vkCopyMemoryToImageEXT vkCopyMemoryToImageEXT = 
                (PFN_vkCopyMemoryToImageEXT)vkGetDeviceProcAddr(mContext->device, "vkCopyMemoryToImageEXT");
        PFN_vkTransitionImageLayoutEXT vkTransitionImageLayoutEXT = 
                (PFN_vkTransitionImageLayoutEXT)vkGetDeviceProcAddr(mContext->device, "vkTransitionImageLayoutEXT");

        vkTransitionImageLayoutEXT(mContext->device, 1, &hostImageLayoutTransitionInfo);

        // Setup host to image copy
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

        // With the image in the correct layout and copy information for all mip levels setup, we can now issue the copy to our taget image from the host
        // The implementation will then convert this to an implementation specific optimal tiling layout
        VkCopyMemoryToImageInfoEXT copyMemoryInfo = {};
        copyMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
        copyMemoryInfo.dstImage = mImage;
        copyMemoryInfo.dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        copyMemoryInfo.regionCount = 1;
        copyMemoryInfo.pRegions = &memoryToImageCopy;

        vkCopyMemoryToImageEXT(mContext->device, &copyMemoryInfo);
    }
    else
    {
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkDeviceSize size = mHeight * bytesPerRow;
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

        mContext->upLoadPool.Execute(upLoadTask);
    }
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

#pragma mark VKRCTexture2D

VKRCTexture2D::VKRCTexture2D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo)
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

bool VKRCTexture2D::IsValid() const
{
    return VKTextureBase::IsValid();
}

#pragma mark VKRCTexture3D

VKRCTexture3D::VKRCTexture3D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo)
    : VKTextureBase(context, imageCreateInfo)
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
    : VKTextureBase(context, imageCreateInfo)
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
    : VKTextureBase(context, imageCreateInfo)
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
