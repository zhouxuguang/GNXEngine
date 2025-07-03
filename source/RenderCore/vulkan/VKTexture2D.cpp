//
//  VKTexture2D.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKTexture2D.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKTexture2D::VKTexture2D(const VulkanContextPtr& context, const TextureDescriptor& des) : Texture2D(des)
{
    mContext = context;
    createTexture(mContext->device, des);
}

VKTexture2D::~VKTexture2D()
{
    release();
}

void VKTexture2D::release()
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

void VKTexture2D::createTexture(const VkDevice device, const TextureDescriptor& des)
{
    bool bUseComponentMapping = false;
    VkComponentMapping componentMapping;
    componentMapping.a = VK_COMPONENT_SWIZZLE_A;
    componentMapping.r = VK_COMPONENT_SWIZZLE_R;
    componentMapping.g = VK_COMPONENT_SWIZZLE_G;
    componentMapping.b = VK_COMPONENT_SWIZZLE_B;
    VkFormat format = VK_FORMAT_UNDEFINED;
    switch (des.format)
    {
        case kTexFormatAlpha8:
            format = VK_FORMAT_R8_UNORM;
            componentMapping.a = VK_COMPONENT_SWIZZLE_R;
            componentMapping.r = VK_COMPONENT_SWIZZLE_ZERO;
            componentMapping.g = VK_COMPONENT_SWIZZLE_ZERO;
            componentMapping.b = VK_COMPONENT_SWIZZLE_ZERO;
            bUseComponentMapping = true;
            break;

        case kTexFormatLuma:
            format = VK_FORMAT_R8_UNORM;
            componentMapping.a = VK_COMPONENT_SWIZZLE_ONE;
            componentMapping.r = VK_COMPONENT_SWIZZLE_R;
            componentMapping.g = VK_COMPONENT_SWIZZLE_R;
            componentMapping.b = VK_COMPONENT_SWIZZLE_R;
            bUseComponentMapping = true;
            break;

        case kTexFormatRGBA4444:
            format = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
            break;

        case kTexFormatRGBA5551:
            format = VK_FORMAT_R5G5B5A1_UNORM_PACK16;
            break;

        case kTexFormatRGB565:
            format = VK_FORMAT_R5G6B5_UNORM_PACK16;
            break;

        case kTexFormatAlphaLum16:
            format = VK_FORMAT_R8G8_UNORM;
            componentMapping.a = VK_COMPONENT_SWIZZLE_G;
            componentMapping.r = VK_COMPONENT_SWIZZLE_R;
            componentMapping.g = VK_COMPONENT_SWIZZLE_R;
            componentMapping.b = VK_COMPONENT_SWIZZLE_R;
            bUseComponentMapping = true;
            break;

        case kTexFormatRGBA32:
            format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
            
        case kTexFormatSRGB8_ALPHA8:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
            
        case kTexFormatRGBA16Float:
            format = VK_FORMAT_R16G16B16A16_SFLOAT;
            break;
            
        case kTexFormatRGBA32Float:
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;

		case kTexFormatDXT1_RGB:
			format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			break;

        default:
            break;
    }

    assert(format != VK_FORMAT_UNDEFINED);
    assert(des.width > 0 && des.height > 0);

	VkFormatProperties3 format_properties_3 = {};
	format_properties_3.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR;

	// Properties3 need to be chained into Properties2
	VkFormatProperties2 format_properties_2 = {};
	format_properties_2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
	format_properties_2.pNext = &format_properties_3;

	// 获得格式的属性
	vkGetPhysicalDeviceFormatProperties2(mContext->physicalDevice, format, &format_properties_2);

    m_bSupportHostImageCopy = mContext->vulkanExtension.enableHostImageCopy;
	if ((format_properties_3.optimalTilingFeatures & VK_FORMAT_FEATURE_2_HOST_IMAGE_TRANSFER_BIT_EXT) == 0)
	{
        m_bSupportHostImageCopy = false;
	}

    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (des.usage & TextureUsageShaderWrite)
    {
        imageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
//    if (des.usage && TextureUsageShaderRead)
//    {
//        imageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
//    }

    if (m_bSupportHostImageCopy)
    {
        imageUsageFlags |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;
    }
    else
    {
        imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    
    VulkanBufferUtil::CreateImage2DVMA(mContext->vmaAllocator, des.width, des.height, format,
                                       VK_SAMPLE_COUNT_1_BIT, 1, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, mImage, mAllocation);
    
    if (mContext->vulkanExtension.enableDebugUtils)
    {
        VkDebugUtilsObjectNameInfoEXT debugNameInfo = {};
        debugNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        debugNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        debugNameInfo.objectHandle = (uint64_t)mImage;
        debugNameInfo.pObjectName = "Texture2d";
        vkSetDebugUtilsObjectNameEXT(mContext->device, &debugNameInfo);
    }
    
#if 0
    VkCommandBuffer commandBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->commandPool);

    VkClearColorValue clearVal;
    clearVal.float32[0] = 0;
    clearVal.float32[1] = 0;
    clearVal.float32[2] = 0;
    clearVal.float32[3] = 0;
    VkImageSubresourceRange imageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(commandBuffer, mImage, VK_IMAGE_LAYOUT_UNDEFINED, &clearVal, 1, &imageSubresourceRange);

    VulkanBufferUtil::EndSingleTimeCommand(mContext->device, mContext->graphicsQueue, mContext->commandPool, commandBuffer);
#endif
    mFormat = format;
    mTextureDes = des;

    //创建图像视图
    VkImageView imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, format, bUseComponentMapping ? &componentMapping : nullptr, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext->device, imageView);
}

void VKTexture2D::SetTextureData(const unsigned char *imageData)
{
    Rect2D rect = {};
    rect.width = mTextureDes.width;
    rect.height = mTextureDes.height;
    ReplaceRegion(rect, imageData, 0);
}

void VKTexture2D::ReplaceRegion(const Rect2D &rect, const unsigned char *imageData, unsigned int mipMapLevel)
{
    if (!imageData)
    {
        return;
    }

    if (m_bSupportHostImageCopy)
    {
        // 图像layout转换
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
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
        PFN_vkCopyMemoryToImageEXT vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT)vkGetDeviceProcAddr(mContext->device, "vkCopyMemoryToImageEXT");
        PFN_vkTransitionImageLayoutEXT vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT)vkGetDeviceProcAddr(mContext->device, "vkTransitionImageLayoutEXT");

		vkTransitionImageLayoutEXT(mContext->device, 1, &hostImageLayoutTransitionInfo);

		// Setup host to image copy
		VkMemoryToImageCopyEXT memory_to_image_copy = {};

		memory_to_image_copy.sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT;
		memory_to_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memory_to_image_copy.imageSubresource.mipLevel = mipMapLevel;
		memory_to_image_copy.imageSubresource.baseArrayLayer = 0;
		memory_to_image_copy.imageSubresource.layerCount = 1;
        memory_to_image_copy.imageOffset.x = rect.offsetX;
        memory_to_image_copy.imageOffset.y = rect.offsetY;
        memory_to_image_copy.imageOffset.z = 0;
		memory_to_image_copy.imageExtent.width = rect.width;
		memory_to_image_copy.imageExtent.height = rect.height;
		memory_to_image_copy.imageExtent.depth = 1;
		memory_to_image_copy.pHostPointer = imageData;

		// With the image in the correct layout and copy information for all mip levels setup, we can now issue the copy to our taget image from the host
		// The implementation will then convert this to an implementation specific optimal tiling layout
		VkCopyMemoryToImageInfoEXT copyMemoryInfo = {};
		copyMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT;
		copyMemoryInfo.dstImage = mImage;
		copyMemoryInfo.dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		copyMemoryInfo.regionCount = 1;
		copyMemoryInfo.pRegions = &memory_to_image_copy;

		vkCopyMemoryToImageEXT(mContext->device, &copyMemoryInfo);
    }
    else
    {
		VkBuffer stageBuffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkDeviceSize size = mTextureDes.height * mTextureDes.bytesPerRow;
		VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			stageBuffer, allocation, nullptr);

		void* data = nullptr;
		vmaMapMemory(mContext->vmaAllocator, allocation, &data);
		memcpy(data, imageData, size);
		vmaUnmapMemory(mContext->vmaAllocator, allocation);

        UpLoadTaskPtr upLoadTask = std::make_shared<UpLoadTask>();
        upLoadTask->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        upLoadTask->subresourceRange.baseMipLevel = mipMapLevel;
        upLoadTask->subresourceRange.baseArrayLayer = 0;
        upLoadTask->subresourceRange.levelCount = 1;
        upLoadTask->subresourceRange.layerCount = 1;

        VkImage image = mImage;
        //mImage = VK_NULL_HANDLE;

        upLoadTask->allocation = allocation;
        upLoadTask->rect = rect;
        upLoadTask->mImage = image;
        upLoadTask->stageBuffer = stageBuffer;
        upLoadTask->mContext = mContext;

        mContext->upLoadPool.Execute(upLoadTask);

		//vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
    }
}

void VKTexture2D::generateMipmapsTexture(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
}

NAMESPACE_RENDERCORE_END
