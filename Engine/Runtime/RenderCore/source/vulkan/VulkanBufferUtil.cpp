//
//  VulkanBufferUtil.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanBufferUtil.h"
#include "VKUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VkCommandBuffer VulkanBufferUtil::BeginSingleTimeCommand(VkDevice device, VkCommandPool cmdPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void VulkanBufferUtil::EndSingleTimeCommand(VulkanContext& context, VkQueue queue,
                                  VkCommandPool cmdPool, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    // 从 FencePool 获取 Fence
    VulkanFencePtr fence = context.fencePool.createFence(context.device);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, fence->getHandle());

    // 等待 Fence 信号
    fence->wait(context.device, UINT64_MAX);

    // 释放 Fence 和命令缓冲区
    context.fencePool.releaseFence(context.device, fence);
    vkFreeCommandBuffers(context.device, cmdPool, 1, &commandBuffer);
}

bool VulkanBufferUtil::IsDepthStencilFormat(VkFormat format)
{
    /*
	* VK_FORMAT_D16_UNORM = 124,
	VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
	VK_FORMAT_D32_SFLOAT = 126,
	VK_FORMAT_S8_UINT = 127,
	VK_FORMAT_D16_UNORM_S8_UINT = 128,
	VK_FORMAT_D24_UNORM_S8_UINT = 129,
	VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
    */
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
        break;
    default:
        break;
    }

    return false;
}

void VulkanBufferUtil::CreateBufferVMA(VmaAllocator vmaAllocator,
                                       StorageMode storageMode,
                                       VkDeviceSize size,
                                       VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlags properties,
                                       VkBuffer& buffer,
                                       VmaAllocation& allocation,
                                       VmaAllocationInfo* allocationInfo)
{
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    if (storageMode == StorageModePrivate)
    {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    else
    {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    allocInfo.requiredFlags = properties;

    vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfo);
}

void VulkanBufferUtil::CreateImage2DVMA(VmaAllocator vmaAllocator,
                                 uint32_t width,
                                 uint32_t height,
                                 VkFormat format,
                                 VkSampleCountFlagBits numSamples,
                                 uint8_t mipLevels,
                                 VkImageTiling tiling,
                                 VkImageUsageFlags usage,
                                 VkImage& image,
                                 VmaAllocation& allocation)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    {
        imageAllocCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    vmaCreateImage(vmaAllocator, &imageInfo, &imageAllocCreateInfo, &image, &allocation, nullptr);
}

void VulkanBufferUtil::CreateImageCube(VmaAllocator vmaAllocator,
                            uint32_t width,
                            uint32_t height,
                            VkFormat format,
                            VkSampleCountFlagBits numSamples,
                            uint8_t mipLevels,
                            VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkImage& image,
                            VmaAllocation& allocation)
{
    // Create optimal tiled target image
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { width, height, 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // Cube faces count as array layers in Vulkan
    imageCreateInfo.arrayLayers = 6;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    vmaCreateImage(vmaAllocator, &imageCreateInfo, &imageAllocCreateInfo, &image, &allocation, nullptr);
}

VkResult VulkanBufferUtil::CreateImageGeneral(VmaAllocator vmaAllocator,
                            const VkImageCreateInfo& imageCreateInfo,
                            VkImage& image,
                            VmaAllocation& allocation)
{
    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    return vmaCreateImage(vmaAllocator, &imageCreateInfo, &imageAllocCreateInfo, &image, &allocation, nullptr);
}

void VulkanBufferUtil::CopyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer,
                                     VkBuffer buffer, VkImage image, int32_t offsetX,
                                     int32_t offsetY, uint32_t width, uint32_t height, uint32_t mipLevel)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {offsetX, offsetY, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(
                           commandBuffer,
                           buffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region
                           );
}

void VulkanBufferUtil::CopyBuffer(VulkanContext& context, VkQueue queue, VkCommandPool cmdPool,
                              VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommand(context.device, cmdPool);

    //拷贝缓冲区
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommand(context, queue, cmdPool, commandBuffer);
}

VkImageView VulkanBufferUtil::CreateImageView(VkDevice device, VkImage image,
                                              VkFormat format, const VkComponentMapping* componentMapping,
                                              VkImageAspectFlags aspectFlags, uint32_t levelCount)
{
    if (VK_NULL_HANDLE == device || VK_NULL_HANDLE == image)
    {
        return VK_NULL_HANDLE;
    }
    
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = levelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (componentMapping)
    {
        viewInfo.components = *componentMapping;
    }
    
    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, NULL, &imageView) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    
    return imageView;
}

static bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanBufferUtil::TransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer,
                                         VkImage image, VkFormat format, VkImageLayout oldLayout,
                                         VkImageLayout newLayout, uint32_t levelCount)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        assert(false);
    }
    
    //针对深度模板缓冲的特殊处理
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    vkCmdPipelineBarrier(
                         commandBuffer,
                         sourceStage, destinationStage,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier
                         );
}

void VulkanBufferUtil::SetImageLayout
 (
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}

void VulkanBufferUtil::SetImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectMask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}

void VulkanBufferUtil::InsertImageMemoryBarrier(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkImageSubresourceRange subresourceRange)
{
    if (!cmdbuffer || !image)
    {
        return;
    }
    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
}

VkFormat VulkanBufferUtil::ConvertTextureFormat(TextureFormat texFormat)
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    switch (texFormat)
    {
        case kTexFormatAlpha8:
            format = VK_FORMAT_R8_UNORM;
            break;

        case kTexFormatLuma:
            format = VK_FORMAT_R8_UNORM;
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
            break;

        case kTexFormatRGBA8:
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
            
        case kTexFormatDepth32FloatStencil8:
            format = VK_FORMAT_D32_SFLOAT_S8_UINT;
            break;
            
        case kTexFormatDepth24Stencil8:
            format = VK_FORMAT_D24_UNORM_S8_UINT;
            break;

		case kTexFormatDepth16:
			format = VK_FORMAT_D16_UNORM;
			break;

		case kTexFormatDepth24:
			format = VK_FORMAT_X8_D24_UNORM_PACK32;
			break;

		case kTexFormatDepth32Float:
			format = VK_FORMAT_D32_SFLOAT;
			break;

        case kTexFormatDXT1_RGB:
            format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            break;
            
        case kTexFormatDXT1_SRGB:
            format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            break;

        case kTexFormatR32Float:
            format = VK_FORMAT_R32_SFLOAT;
            break;

        case kTexFormatRG32Uint:
            format = VK_FORMAT_R32G32_UINT;
            break;

        case kTexFormatRG32Sint:
            format = VK_FORMAT_R32G32_SINT;
            break;

        case kTexFormatRG32Float:
            format = VK_FORMAT_R32G32_SFLOAT;
            break;

        default:
            break;
    }
    
    return format;
}

/*
 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
 */
VkImageUsageFlags VulkanBufferUtil::ConvertTextureUsage(TextureUsage textureUsage, VkFormat format)
{
    
    VkImageUsageFlags flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    switch (textureUsage)
    {
        case TextureUsage::TextureUsageShaderRead:
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            
        case TextureUsage::TextureUsageShaderWrite:
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
            
        case TextureUsage::TextureUsageRenderTarget:
            // 还要区分是深度模板还是颜色缓冲
            if (VulkanBufferUtil::IsDepthStencilFormat(format))
            {
                flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            else
            {
                flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            
        default:
            break;
    }
    
    return flags;
}

uint32_t VulkanBufferUtil::GetFormatSize(VkFormat format)
{
    uint32_t result = 0;
    switch (format) 
    {
    case VK_FORMAT_UNDEFINED:
      result = 0;
      break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
      result = 1;
      break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R8_UNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_SNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_USCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_SSCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_UINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SRGB:
      result = 1;
      break;
    case VK_FORMAT_R8G8_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_UINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SRGB:
      result = 2;
      break;
    case VK_FORMAT_R8G8B8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_UINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_R16_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_UINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SFLOAT:
      result = 2;
      break;
    case VK_FORMAT_R16G16_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_UINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R16G16B16_UNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_USCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SSCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_UINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SFLOAT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16A16_UNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_USCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SSCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_UINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32_UINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R32G32_UINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32G32B32_UINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SFLOAT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32A32_UINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64_UINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R64G64_UINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64G64B64_UINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SFLOAT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64A64_UINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      result = 32;
      break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
      result = 4;
      break;

    default:
      break;
    }
    return result;
}

VkImageAspectFlags VulkanBufferUtil::GetImageAspectFlags(VkFormat format) 
{
	switch (format) 
    {
		// 纯深度格式
	    case VK_FORMAT_D16_UNORM:
	    case VK_FORMAT_D32_SFLOAT:
	    case VK_FORMAT_X8_D24_UNORM_PACK32:
		    return VK_IMAGE_ASPECT_DEPTH_BIT;

		// 纯模板格式
	    case VK_FORMAT_S8_UINT:
		    return VK_IMAGE_ASPECT_STENCIL_BIT;

		// 深度-模板组合格式
	    case VK_FORMAT_D16_UNORM_S8_UINT:
	    case VK_FORMAT_D24_UNORM_S8_UINT:
	    case VK_FORMAT_D32_SFLOAT_S8_UINT:
		    return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

		// 颜色格式
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}

    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkFormat VulkanBufferUtil::ConvertVertexFormat(VertexFormat format)
{
    switch (format)
    {
        // 浮点格式
        case VertexFormatFloat:      return VK_FORMAT_R32_SFLOAT;
        case VertexFormatFloat2:     return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormatFloat3:     return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormatFloat4:     return VK_FORMAT_R32G32B32A32_SFLOAT;
        
        // 整数格式（不归一化）
        case VertexFormatInt:        return VK_FORMAT_R32_SINT;
        case VertexFormatInt2:       return VK_FORMAT_R32G32_SINT;
        case VertexFormatInt3:       return VK_FORMAT_R32G32B32_SINT;
        case VertexFormatInt4:       return VK_FORMAT_R32G32B32A32_SINT;
        
        case VertexFormatUInt:       return VK_FORMAT_R32_UINT;
        case VertexFormatUInt2:      return VK_FORMAT_R32G32_UINT;
        case VertexFormatUInt3:      return VK_FORMAT_R32G32B32_UINT;
        case VertexFormatUInt4:      return VK_FORMAT_R32G32B32A32_UINT;
        
        // 半精度浮点
        case VertexFormatHalfFloat:  return VK_FORMAT_R16_SFLOAT;
        case VertexFormatHalfFloat2: return VK_FORMAT_R16G16_SFLOAT;
        case VertexFormatHalfFloat3: return VK_FORMAT_R16G16B16_SFLOAT;
        case VertexFormatHalfFloat4: return VK_FORMAT_R16G16B16A16_SFLOAT;
        
        // 无符号字节（不归一化）
        case VertexFormatUChar:      return VK_FORMAT_R8_UINT;
        case VertexFormatUChar2:     return VK_FORMAT_R8G8_UINT;
        case VertexFormatUChar3:     return VK_FORMAT_R8G8B8_UINT;
        case VertexFormatUChar4:     return VK_FORMAT_R8G8B8A8_UINT;
        
        // 有符号字节（不归一化）
        case VertexFormatChar:       return VK_FORMAT_R8_SINT;
        case VertexFormatChar2:      return VK_FORMAT_R8G8_SINT;
        case VertexFormatChar3:      return VK_FORMAT_R8G8B8_SINT;
        case VertexFormatChar4:      return VK_FORMAT_R8G8B8A8_SINT;
        
        // 无符号短整数（不归一化）
        case VertexFormatUShort:     return VK_FORMAT_R16_UINT;
        case VertexFormatUShort2:    return VK_FORMAT_R16G16_UINT;
        case VertexFormatUShort3:    return VK_FORMAT_R16G16B16_UINT;
        case VertexFormatUShort4:    return VK_FORMAT_R16G16B16A16_UINT;
        
        // 有符号短整数（不归一化）
        case VertexFormatShort:      return VK_FORMAT_R16_SINT;
        case VertexFormatShort2:     return VK_FORMAT_R16G16_SINT;
        case VertexFormatShort3:     return VK_FORMAT_R16G16B16_SINT;
        case VertexFormatShort4:     return VK_FORMAT_R16G16B16A16_SINT;
        
        // ★★★ 无符号归一化格式: uint8 [0,255] → float [0,1] ★★★
        case VertexFormatUCharNorm:   return VK_FORMAT_R8_UNORM;
        case VertexFormatUChar2Norm:  return VK_FORMAT_R8G8_UNORM;
        case VertexFormatUChar3Norm:  return VK_FORMAT_R8G8B8_UNORM;
        case VertexFormatUChar4Norm:  return VK_FORMAT_R8G8B8A8_UNORM;
        
        // ★★★ 有符号归一化格式: int8 [-128,127] → float [-1,1] ★★★
        case VertexFormatCharNorm:    return VK_FORMAT_R8_SNORM;
        case VertexFormatChar2Norm:   return VK_FORMAT_R8G8_SNORM;
        case VertexFormatChar3Norm:   return VK_FORMAT_R8G8B8_SNORM;   // 法线归一化常用
        case VertexFormatChar4Norm:   return VK_FORMAT_R8G8B8A8_SNORM;
        
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

uint32_t VulkanBufferUtil::GetVertexFormatSize(VertexFormat format)
{
    switch (format)
    {
        // 浮点格式
        case VertexFormatFloat:      return 4;
        case VertexFormatFloat2:     return 8;
        case VertexFormatFloat3:     return 12;
        case VertexFormatFloat4:     return 16;
        
        // 整数格式
        case VertexFormatInt:
        case VertexFormatUInt:       return 4;
        case VertexFormatInt2:
        case VertexFormatUInt2:      return 8;
        case VertexFormatInt3:
        case VertexFormatUInt3:      return 12;
        case VertexFormatInt4:
        case VertexFormatUInt4:      return 16;
        
        // 半精度浮点
        case VertexFormatHalfFloat:  return 2;
        case VertexFormatHalfFloat2: return 4;
        case VertexFormatHalfFloat3: return 6;
        case VertexFormatHalfFloat4: return 8;
        
        // 单字节（归一化与非归一化相同）
        case VertexFormatUChar:
        case VertexFormatChar:
        case VertexFormatUCharNorm:
        case VertexFormatCharNorm:   return 1;
        
        case VertexFormatUChar2:
        case VertexFormatChar2:
        case VertexFormatUChar2Norm:
        case VertexFormatChar2Norm:  return 2;
        
        case VertexFormatUChar3:
        case VertexFormatChar3:
        case VertexFormatUChar3Norm:
        case VertexFormatChar3Norm:  return 3;
        
        case VertexFormatUChar4:
        case VertexFormatChar4:
        case VertexFormatUChar4Norm:
        case VertexFormatChar4Norm:  return 4;
        
        // 双字节
        case VertexFormatUShort:
        case VertexFormatShort:      return 2;
        case VertexFormatUShort2:
        case VertexFormatShort2:     return 4;
        case VertexFormatUShort3:
        case VertexFormatShort3:     return 6;
        case VertexFormatUShort4:
        case VertexFormatShort4:     return 8;
        
        default:
            return 0;
    }
}

NAMESPACE_RENDERCORE_END
