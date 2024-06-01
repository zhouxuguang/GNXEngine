//
//  VKTextureCube.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKTextureCube.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKTextureCube::VKTextureCube(VulkanContextPtr context, const std::vector<TextureDescriptor>& desArray) : TextureCube(desArray)
{
    mContext = context;
    uint32_t width = desArray[0].width;
    uint32_t height = desArray[0].height;
    
    mTextureDesc = desArray[0];
    
    VkFormat format = VulkanBufferUtil::ConvertTextureFormat(mTextureDesc.format);

    assert(format != VK_FORMAT_UNDEFINED);
    
    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    
    VulkanBufferUtil::CreateImageCube(mContext->vmaAllocator, width, height, format,
                                      VK_SAMPLE_COUNT_1_BIT, 1, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, mImage, mAllocation);
    
    //创建图像视图
    VkImageView imageView = 0;
    
    // Create image view
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // Cube map view type
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    // 6 array layers (faces)
    viewCreateInfo.subresourceRange.layerCount = 6;
    // Set number of mip levels
    viewCreateInfo.subresourceRange.levelCount = 1;   //这里先写死了
    viewCreateInfo.image = mImage;
    vkCreateImageView(mContext->device, &viewCreateInfo, nullptr, &imageView);
    
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext->device, imageView);
}

VKTextureCube::~VKTextureCube()
{
    Release();
}

void VKTextureCube::setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData)
{
    if (!imageData || imageSize == 0)
    {
        return;
    }
    
    //申请临时内存进行数据传输
    VkBuffer stageBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, imageSize,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      stageBuffer, allocation, nullptr);
    
    void *data = nullptr;
    vmaMapMemory(mContext->vmaAllocator, allocation, &data);
    memcpy(data, imageData, imageSize);
    vmaUnmapMemory(mContext->vmaAllocator, allocation);
    
    uint32_t face = (uint32_t)cubeFace;
    
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.baseArrayLayer = face;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = mTextureDesc.width;
    bufferCopyRegion.imageExtent.height = mTextureDesc.height;
    bufferCopyRegion.imageExtent.depth = 1;
    bufferCopyRegion.bufferOffset = 0;

    // Image barrier for optimal image (target)
    // Set initial layout for single array layers (face) of the optimal (target) tiled texture
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    
    VkCommandBuffer commandBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->commandPool);

    VulkanBufferUtil::SetImageLayout(
                                     commandBuffer,
                                    mImage,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    subresourceRange);

    // Copy the cube map faces from the staging buffer to the optimal tiled image
    vkCmdCopyBufferToImage(
                           commandBuffer,
                           stageBuffer,
        mImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopyRegion
        );

    // Change texture image layout to shader read after all faces have been copied
    VulkanBufferUtil::SetImageLayout(
                               commandBuffer,
                               mImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                subresourceRange);

    VulkanBufferUtil::EndSingleTimeCommand(mContext->device, mContext->graphicsQueue, mContext->commandPool, commandBuffer);
    
    vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
    
}

bool VKTextureCube::isValid() const
{
    return mImage != VK_NULL_HANDLE;
}

void VKTextureCube::Release()
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

NAMESPACE_RENDERCORE_END
