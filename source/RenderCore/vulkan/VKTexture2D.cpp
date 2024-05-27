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

        default:
            break;
    }

    assert(format != VK_FORMAT_UNDEFINED);
    assert(des.width > 0 && des.height > 0);

    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    VulkanBufferUtil::CreateImage2DVMA(mContext->vmaAllocator, des.width, des.height, format,
                                       VK_SAMPLE_COUNT_1_BIT, 1, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, mImage, mAllocation);

    VkCommandBuffer commandBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->commandPool);

    VkClearColorValue clearVal;
    clearVal.float32[0] = 0;
    clearVal.float32[1] = 0;
    clearVal.float32[2] = 0;
    clearVal.float32[3] = 0;
    VkImageSubresourceRange imageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(commandBuffer, mImage, VK_IMAGE_LAYOUT_UNDEFINED, &clearVal, 1, &imageSubresourceRange);

    VulkanBufferUtil::EndSingleTimeCommand(mContext->device, mContext->graphicsQueue, mContext->commandPool, commandBuffer);
    mFormat = format;
    mTextureDes = des;

    //创建图像视图
    VkImageView imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, format, bUseComponentMapping ? &componentMapping : nullptr, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext->device, imageView);
}

void VKTexture2D::setTextureData(const unsigned char *imageData)
{
    Rect2D rect = {};
    rect.width = mTextureDes.width;
    rect.height = mTextureDes.height;
    replaceRegion(rect, imageData, 0);
}



void VKTexture2D::replaceRegion(const Rect2D &rect, const unsigned char *imageData, unsigned int mipMapLevel)
{
    if (!imageData)
    {
        return;
    }
    
    VkBuffer stageBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = mTextureDes.height * mTextureDes.bytesPerRow;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      stageBuffer, allocation, nullptr);
    
    void *data = nullptr;
    vmaMapMemory(mContext->vmaAllocator, allocation, &data);
    memcpy(data, imageData, size);
    vmaUnmapMemory(mContext->vmaAllocator, allocation);
    
    VkCommandBuffer commandBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->commandPool);
    
    VulkanBufferUtil::CopyBufferToImage(mContext->device, commandBuffer, stageBuffer, mImage, 
                                        rect.offsetX, rect.offsetY, rect.width, rect.height, mipMapLevel);
    
    VulkanBufferUtil::EndSingleTimeCommand(mContext->device, mContext->graphicsQueue, mContext->commandPool, commandBuffer);
    
    vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
}



void VKTexture2D::generateMipmapsTexture(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
}

NAMESPACE_RENDERCORE_END
