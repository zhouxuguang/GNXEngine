//
//  VKBlitEncoder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#include "VKBlitEncoder.h"
#include "VKVertexBuffer.h"
#include "VKRCBuffer.h"
#include "VKTextureBase.h"

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

VkBuffer VKBlitEncoder::GetVkBuffer(VertexBufferPtr buffer) const
{
    if (!buffer)
    {
        return VK_NULL_HANDLE;
    }
    
    // 尝试转换为VKVertexBuffer
    VKVertexBufferPtr vkVertexBuffer = std::dynamic_pointer_cast<VKVertexBuffer>(buffer);
    if (vkVertexBuffer)
    {
        return vkVertexBuffer->GetGpuBuffer();
    }
    
    return VK_NULL_HANDLE;
}

VkBuffer VKBlitEncoder::GetVkBufferFromRC(RCBufferPtr buffer) const
{
    if (!buffer)
    {
        return VK_NULL_HANDLE;
    }
    
    // 尝试转换为VKRCBuffer
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

void VKBlitEncoder::CopyBufferToBuffer(VertexBufferPtr source,
                                       uint64_t sourceOffset,
                                       VertexBufferPtr destination,
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

void VKBlitEncoder::FillBuffer(VertexBufferPtr destination,
                              uint64_t destinationOffset,
                              const void* data,
                              uint64_t dataSize)
{
    // Vulkan不支持直接fillBuffer
    // 需要通过临时buffer拷贝或使用compute shader实现
    // 这里使用CopyBufferToBuffer作为简化实现
    
    // 注意：这不是真正的fill操作，实际项目中应该使用compute shader或临时buffer
    if (!data || dataSize == 0)
    {
        return;
    }
    
    // TODO: 实现真正的fill操作
    // 可以创建一个临时的buffer填充pattern，然后拷贝
    
    //vkCmdFillBuffer()
}

// ==================== RCBuffer操作（新接口） ====================

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
    
    VkBuffer srcBuffer = GetVkBufferFromRC(source);
    VkBuffer dstBuffer = GetVkBufferFromRC(destination);
    
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

void VKBlitEncoder::CopyTextureToBuffer(RCTexturePtr source,
                                        uint32_t sourceSlice,
                                        uint32_t sourceMipLevel,
                                        const Rect2D& sourceOffset,
                                        const Rect2D& sourceSize,
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
    VkBuffer dstBuffer = GetVkBufferFromRC(destination);
    
    if (srcImage == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkImageSubresourceLayers subresource = GetImageSubresourceLayers(source, sourceSlice, sourceMipLevel);
    
    VkOffset3D srcOffset = {sourceOffset.offsetX, sourceOffset.offsetY, 0};
    VkExtent3D extent = {static_cast<uint32_t>(sourceSize.width), 
                         static_cast<uint32_t>(sourceSize.height), 1};
    
    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = destinationOffset;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
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
                                        const Rect2D& destinationOffset,
                                        const Rect2D& destinationSize)
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    VkBuffer srcBuffer = GetVkBufferFromRC(source);
    VkImage dstImage = GetVkImage(destination);
    
    if (srcBuffer == VK_NULL_HANDLE || dstImage == VK_NULL_HANDLE)
    {
        return;
    }
    
    VkImageSubresourceLayers subresource = GetImageSubresourceLayers(destination, 
                                                                    destinationSlice, 
                                                                    destinationMipLevel);
    
    VkOffset3D dstOffset = {destinationOffset.offsetX, destinationOffset.offsetY, 0};
    VkExtent3D extent = {static_cast<uint32_t>(destinationSize.width), 
                         static_cast<uint32_t>(destinationSize.height), 1};
    
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

// ==================== Texture到Buffer操作 ====================

void VKBlitEncoder::CopyTextureToBuffer(RCTexturePtr source,
                                       uint32_t sourceSlice,
                                       uint32_t sourceMipLevel,
                                       const Rect2D& sourceOffset,
                                       const Rect2D& sourceSize,
                                       VertexBufferPtr destination,
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
    
    VkOffset3D srcOffset = {sourceOffset.offsetX, sourceOffset.offsetY, 0};
    VkExtent3D extent = {static_cast<uint32_t>(sourceSize.width), 
                         static_cast<uint32_t>(sourceSize.height), 1};
    
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource = subresource;
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstSubresource = subresource;
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = extent;
    
    // 注意：Vulkan的CopyImageToBuffer需要dstOffset
    // 但这里我们使用destinationOffset参数
    // 实际使用时需要调整buffer offset
    
//    vkCmdCopyImageToBuffer(mCommandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                          dstBuffer, 1, &copyRegion);
}

// ==================== Buffer到Texture操作 ====================

void VKBlitEncoder::CopyBufferToTexture(VertexBufferPtr source,
                                       uint64_t sourceOffset,
                                       uint64_t sourceBytesPerRow,
                                       uint64_t sourceBytesPerImage,
                                       RCTexturePtr destination,
                                       uint32_t destinationSlice,
                                       uint32_t destinationMipLevel,
                                       const Rect2D& destinationOffset,
                                       const Rect2D& destinationSize)
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
    
    VkOffset3D dstOffset = {destinationOffset.offsetX, destinationOffset.offsetY, 0};
    VkExtent3D extent = {static_cast<uint32_t>(destinationSize.width), 
                         static_cast<uint32_t>(destinationSize.height), 1};
    
    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = sourceOffset;
    copyRegion.bufferRowLength = 0;  // 表示紧密打包
    copyRegion.bufferImageHeight = 0; // 表示紧密打包
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
                                         const Rect2D& sourceOffset,
                                         const Rect2D& sourceSize,
                                         RCTexturePtr destination,
                                         uint32_t destinationSlice,
                                         uint32_t destinationMipLevel,
                                         const Rect2D& destinationOffset,
                                         const Rect2D& destinationSize)
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
    
    VkOffset3D srcOffset = {sourceOffset.offsetX, sourceOffset.offsetY, 0};
    VkOffset3D dstOffset = {destinationOffset.offsetX, destinationOffset.offsetY, 0};
    VkExtent3D extent = {static_cast<uint32_t>(sourceSize.width), 
                         static_cast<uint32_t>(sourceSize.height), 1};
    
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

// ==================== Barrier操作 ====================

void VKBlitEncoder::MemoryBarrier()
{
    if (!mContext || !mCommandBuffer)
    {
        return;
    }
    
    // 创建内存屏障，确保之前的所有操作都已完成
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(mCommandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
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
