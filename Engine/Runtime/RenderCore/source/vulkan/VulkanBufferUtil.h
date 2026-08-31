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

struct VulkanContext;

NAMESPACE_RENDERCORE_END

#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

// buffer和图像操作的工具函数
class VulkanBufferUtil
{
public:
    static VkCommandBuffer BeginSingleTimeCommand(VkDevice device, VkCommandPool cmdPool);

    static void EndSingleTimeCommand(VulkanContext& context, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer commandBuffer);

    static bool IsDepthStencilFormat(VkFormat format);
    
    static bool IsSRGBFormat(VkFormat format);

    /// 判断是否为 ASTC LDR（UNORM/SRGB block）格式
    static bool IsASTCLDRFormat(VkFormat format);

    /// 判断是否为 ASTC 格式（LDR 或 HDR）
    static bool IsASTCFormat(VkFormat format);

    /**
     创建图像视图
     @param device 设备句柄
     @param image 图像
     @param format 图像格式
     @param componentMapping 组件映射
     @param aspectFlags 方面标记
     @param levelCount mip 等级数
     @param astcDecodeMode 非空时，为 ASTC LDR 格式指定解码模式（VK_EXT_astc_decode_mode）
     */
    static VkImageView CreateImageView(VkDevice device,
                                       VkImage image,
                                       VkFormat format,
                                       const VkComponentMapping* componentMapping,
                                       VkImageAspectFlags aspectFlags,
                                       uint32_t levelCount,
                                       const VkImageViewASTCDecodeModeEXT* astcDecodeMode = nullptr);

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
    
    // 创建纹理的实用函数
    static VkResult CreateImageGeneral(VmaAllocator vmaAllocator,
                                const VkImageCreateInfo& imageCreateInfo,
                                VkImage& image,
                                VmaAllocation& allocation);

    //拷贝buffer到图像
    static void CopyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer,
                           VkBuffer buffer, VkImage image, int32_t offsetX, int32_t offsetY,
                           uint32_t width, uint32_t height, uint32_t mipLevel,
                           uint32_t baseArrayLayer = 0);

    //buffer之间的拷贝
    static void CopyBuffer(VulkanContext& context, VkQueue queue, VkCommandPool cmdPool,
                    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

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
    
    static VkImageUsageFlags ConvertTextureUsage(TextureUsage textureUsage, VkFormat format);
    
    // 获得格式对应的大小
    static uint32_t GetFormatSize(VkFormat format);

    static VkImageAspectFlags GetImageAspectFlags(VkFormat format);
    
    // VertexFormat 转 VkFormat
    static VkFormat ConvertVertexFormat(VertexFormat format);
    
    // 获取 VertexFormat 的大小（字节数）
    static uint32_t GetVertexFormatSize(VertexFormat format);
};

NAMESPACE_RENDERCORE_END

#endif
