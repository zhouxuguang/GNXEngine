//
//  VulkanBufferUtil.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_RENDER_CORE_VULKAN_BUFFER_UTIL_INCLUDE_JHDSGHHJDHSFJHDSG
#define GNX_ENGINE_RENDER_CORE_VULKAN_BUFFER_UTIL_INCLUDE_JHDSGHHJDHSFJHDSG

#include "VKRenderDefine.h"
#include "RenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

// buffer和图像操作的工具函数
class VulkanBufferUtil
{
public:
    static VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool cmdPool);
    
    static void EndSingleTimeCommand(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer commandBuffer);
    
    /**
     @param vmaAllocator 设备句柄
     @param storageMode  访问模式
     @param size  缓冲区大小
     @param usage 缓冲区用途
     @param properties  内存属性
     @param buffer  返回的buffer
     @param allocation  内存分配的信息
     @param allocationInfo  内存分配的信息
     @return 返回的buffer句柄
     */
    static void CreateBufferVMA(VmaAllocator vmaAllocator,
                                StorageMode storageMode,
                                    VkDeviceSize size,
                                    VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags properties,
                                    VkBuffer& buffer,
                                    VmaAllocation& allocation,
                                    VmaAllocationInfo* allocationInfo);

    //创建2d图像
    static void CreateImage2DVMA(VmaAllocator vmaAllocator,
                              uint32_t width,
                              uint32_t height,
                              VkFormat format,
                              VkSampleCountFlagBits numSamples,
                              uint8_t mipLevels,
                              VkImageTiling tiling,
                              VkImageUsageFlags usage,
                              VkImage& image,
                              VmaAllocation& allocation);
    
    // 创建立方体贴图
    static void CreateImageCube(VmaAllocator vmaAllocator,
                                uint32_t width,
                                uint32_t height,
                                VkFormat format,
                                VkSampleCountFlagBits numSamples,
                                uint8_t mipLevels,
                                VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkImage& image,
                                VmaAllocation& allocation);

    //拷贝buffer到图像
    static void CopyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer,
                           VkBuffer buffer, VkImage image, int32_t offsetX, int32_t offsetY, uint32_t width, uint32_t height, uint32_t mipLevel);

    //buffer之间的拷贝
    static void CopyBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool,
                    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    //创建图像视图
    static VkImageView CreateImageView(VkDevice device,
                                       VkImage image,
                                       VkFormat format,
                                       const VkComponentMapping* componentMapping,
                                       VkImageAspectFlags aspectFlags, uint32_t levelCount);

    //图像布局格式之间的转换
    static void TransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer,
                               VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t levelCount = 1);
    
    static void SetImageLayout
     (
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    
    static void SetImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    //插入图像屏障
    static void InsertImageMemoryBarrier(
            VkCommandBuffer cmdbuffer,
            VkImage image,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkImageSubresourceRange subresourceRange);
    
    static VkFormat ConvertTextureFormat(TextureFormat texFormat);
};

NAMESPACE_RENDERCORE_END

#endif
