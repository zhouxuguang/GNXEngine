//
//  VKBlitEncoder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#include "VKBlitEncoder.h"
#include "VKRCBuffer.h"
#include "VKTextureBase.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKBlitEncoder::VKBlitEncoder(VulkanContextPtr context, VkCommandBuffer commandBuffer)
    : mContext(context)
    , mCommandBuffer(commandBuffer)
{
}

VKBlitEncoder::~VKBlitEncoder()
{
    mContext = nullptr;
    mCommandBuffer = VK_NULL_HANDLE;
}

VkBuffer VKBlitEncoder::GetVkBuffer(RCBufferPtr buffer) const
{
    if (!buffer)
    {
        return VK_NULL_HANDLE;
    }
    
    VKRCBufferPtr vkRCBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (vkRCBuffer)
    {
        return vkRCBuffer->GetVkBuffer();
    }
    
    return VK_NULL_HANDLE;
}

VkImage VKBlitEncoder::GetVkImage(RCTexturePtr texture) const
{
    if (!texture)
    {
        return VK_NULL_HANDLE;
    }
    
    // 尝试转换为VKTextureBase
    VKTextureBasePtr vkTexture = std::dynamic_pointer_cast<VKTextureBase>(texture);
    if (vkTexture)
    {
        return vkTexture->GetVKImage();
    }
    
    return VK_NULL_HANDLE;
}

VkImageSubresourceLayers VKBlitEncoder::GetImageSubresourceLayers(RCTexturePtr texture,
                                                                    uint32_t slice,
                                                                    uint32_t mipLevel) const
{
    VkImageSubresourceLayers subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = mipLevel;
    subresource.baseArrayLayer = slice;
    subresource.layerCount = 1;
    
    return subresource;
}

// ==================== Buffer操作 ====================

void VKBlitEncoder::CopyBuffer(RCBufferPtr source,
                               uint64_t sourceOffset,
                               RCBufferPtr destination,
                               uint64_t destinationOffset,
                               uint64_t size)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkBuffer srcBuffer = GetVkBuffer(source);
    VkBuffer dstBuffer = GetVkBuffer(destination);
    
    if (srcBuffer == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = sourceOffset;
    copyRegion.dstOffset = destinationOffset;
    copyRegion.size = size;
    
    vkCmdCopyBuffer(mCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void VKBlitEncoder::FillBuffer(RCBufferPtr destination,
                               uint64_t destinationOffset,
                               const void* data,
                               uint64_t dataSize)
{
    auto dst = std::dynamic_pointer_cast<VKRCBuffer>(destination);
    if (!dst || !dst->IsValid() || !data || dataSize == 0 ||
        destinationOffset > dst->GetSize() || dataSize > (uint64_t)dst->GetSize() - destinationOffset)
    {
        return;
    }

    RCBufferDesc stagingDesc((uint32_t)dataSize, RCBufferUsage::TransferSrc, StorageModeShared);
    auto staging = std::make_shared<VKRCBuffer>(mContext, stagingDesc, data);
    if (!staging->IsValid())
    {
        return;
    }

    CopyBuffer(staging, 0, destination, destinationOffset, dataSize);
    // VKRCBuffer 析构通过 VulkanGarbageCollector 延迟销毁，覆盖本次提交的 GPU 生命周期。
}

void VKBlitEncoder::CopyTextureToBuffer(RCTexturePtr source,
                                        uint32_t sourceSlice,
                                        uint32_t sourceMipLevel,
                                        const mathutil::Vector2i& sourceOffset,
                                        const mathutil::Vector2i& sourceSize,
                                        RCBufferPtr destination,
                                        uint64_t destinationOffset,
                                        uint64_t destinationBytesPerRow,
                                        uint64_t destinationBytesPerImage)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkImage srcImage = GetVkImage(source);
    VkBuffer dstBuffer = GetVkBuffer(destination);
    
    if (srcImage == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkImageSubresourceLayers subresource = GetImageSubresourceLayers(source, sourceSlice, sourceMipLevel);
    
    VkOffset3D srcOffset = {sourceOffset.x, sourceOffset.y, 0};
    VkExtent3D extent = {static_cast<uint32_t>(sourceSize.x), 
                         static_cast<uint32_t>(sourceSize.y), 1};
    
    // Vulkan 的 bufferRowLength 以 texel 为单位。
    uint32_t texelSize = 0;
    VKTextureBasePtr vkSource = std::dynamic_pointer_cast<VKTextureBase>(source);
    if (vkSource)
    {
        texelSize = VulkanBufferUtil::GetFormatSize(vkSource->GetVKFormat());
    }

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = destinationOffset;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    if (texelSize > 0 && destinationBytesPerRow >= texelSize)
    {
        copyRegion.bufferRowLength = (uint32_t)(destinationBytesPerRow) / texelSize;
        if (copyRegion.bufferRowLength > 0 && destinationBytesPerImage >= destinationBytesPerRow)
        {
            copyRegion.bufferImageHeight = (uint32_t)(destinationBytesPerImage / destinationBytesPerRow);
        }
    }
    copyRegion.imageSubresource = subresource;
    copyRegion.imageOffset = srcOffset;
    copyRegion.imageExtent = extent;
    
    vkCmdCopyImageToBuffer(mCommandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          dstBuffer, 1, &copyRegion);
}

void VKBlitEncoder::CopyBufferToTexture(RCBufferPtr source,
                                        uint64_t sourceOffset,
                                        uint64_t sourceBytesPerRow,
                                        uint64_t sourceBytesPerImage,
                                        RCTexturePtr destination,
                                        uint32_t destinationSlice,
                                        uint32_t destinationMipLevel,
                                        const mathutil::Vector2i& destinationOffset,
                                        const mathutil::Vector2i& destinationSize)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkBuffer srcBuffer = GetVkBuffer(source);
    VkImage dstImage = GetVkImage(destination);
    
    if (srcBuffer == VK_NULL_HANDLE || dstImage == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkImageSubresourceLayers subresource = GetImageSubresourceLayers(destination, 
                                                                    destinationSlice, 
                                                                    destinationMipLevel);
    
    VkOffset3D dstOffset = {destinationOffset.x, destinationOffset.y, 0};
    VkExtent3D extent = {static_cast<uint32_t>(destinationSize.x), 
                         static_cast<uint32_t>(destinationSize.y), 1};
    
    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = sourceOffset;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource = subresource;
    copyRegion.imageOffset = dstOffset;
    copyRegion.imageExtent = extent;
    
    vkCmdCopyBufferToImage(mCommandBuffer, srcBuffer, dstImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}

// ==================== Texture到Texture操作 ====================

void VKBlitEncoder::CopyTextureToTexture(RCTexturePtr source,
                                         uint32_t sourceSlice,
                                         uint32_t sourceMipLevel,
                                         const mathutil::Vector2i& sourceOffset,
                                         const mathutil::Vector2i& sourceSize,
                                         RCTexturePtr destination,
                                         uint32_t destinationSlice,
                                         uint32_t destinationMipLevel,
                                         const mathutil::Vector2i& destinationOffset,
                                         const mathutil::Vector2i& destinationSize)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkImage srcImage = GetVkImage(source);
    VkImage dstImage = GetVkImage(destination);
    
    if (srcImage == VK_NULL_HANDLE || dstImage == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkImageSubresourceLayers srcSubresource = GetImageSubresourceLayers(source, sourceSlice, sourceMipLevel);
    VkImageSubresourceLayers dstSubresource = GetImageSubresourceLayers(destination, destinationSlice, destinationMipLevel);
    
    VkOffset3D srcOffset = {sourceOffset.x, sourceOffset.y, 0};
    VkOffset3D dstOffset = {destinationOffset.x, destinationOffset.y, 0};
    VkExtent3D extent = {static_cast<uint32_t>(sourceSize.x), 
                         static_cast<uint32_t>(sourceSize.y), 1};
    
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource = srcSubresource;
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstSubresource = dstSubresource;
    copyRegion.dstOffset = dstOffset;
    copyRegion.extent = extent;
    
    vkCmdCopyImage(mCommandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}

// ==================== Mipmap操作 ====================

void VKBlitEncoder::GenerateMipmaps(RCTexturePtr texture, uint32_t slice)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkImage image = GetVkImage(texture);
    if (image == VK_NULL_HANDLE)
    {
        return;
    }
    
    // 获取纹理属性
    // TODO: 实现完整的mipmap生成逻辑
    // 需要知道纹理的mipLevels、width、height等信息
    // 这里使用vkCmdBlitImage生成mipmap
    
    // 注意：实际实现需要从纹理对象获取更多信息
    // 包括format、mipLevels、width、height等
}

void VKBlitEncoder::GenerateMipmapsForRange(RCTexturePtr texture,
                                             uint32_t slice,
                                             uint32_t baseMipLevel,
                                             uint32_t levelCount)
{
    // TODO: 实现指定范围的mipmap生成
    GenerateMipmaps(texture, slice);
}

// ==================== 编码结束 ====================

void VKBlitEncoder::EndEncode()
{
    // Vulkan的CommandBuffer不需要显式结束blit encoding
    // 所有的encoder都是直接记录到同一个command buffer中
    // 这个函数主要用于接口一致性
    mContext = nullptr;
    mCommandBuffer = VK_NULL_HANDLE;
}

NAMESPACE_RENDERCORE_END
