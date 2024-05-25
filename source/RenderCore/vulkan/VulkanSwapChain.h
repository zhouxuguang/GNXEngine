//
//  VulkanSwapChain.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_VULKAN_SWAPCHAIN_INCLUDE_JDSGHVDHGH
#define GNX_ENGINE_VULKAN_SWAPCHAIN_INCLUDE_JDSGHVDHGH

#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

class VulkanSwapChain
{
public:
    VulkanSwapChain(VulkanContextPtr vulkanContext, uint32_t width, uint32_t height);

    ~VulkanSwapChain();

    void Release();
    
    void CreateFrameBuffer(VulkanContextPtr vulkanContext, VkRenderPass renderPass, VkImageView depthStencilImage);
    
    //创建交换链
    void CreateSwapChain(VulkanContextPtr vulkanContext, uint32_t width, uint32_t height);
    
    size_t GetSwapChainImageCount() const
    {
        return (size_t)mSwapchainImageCount;
    }
    
    VkFormat GetDisplayFormat() const
    {
        return mDisplayFormat;
    }
    
    uint32_t GetWidth() const
    {
        return mDisplaySize.width;
    }
    
    uint32_t GetHeight() const
    {
        return mDisplaySize.height;
    }
    
    const VkSwapchainKHR GetSwapChain() const
    {
        return mSwapchain;
    }
    
    VkFramebuffer GetFrameBuffer(uint32_t index) const
    {
        return mFrameBuffers[index];
    }

    VkImage GetImage(uint32_t index) const
    {
        return mDisplayImages[index];
    }
    
    VkImageView GetImageView(uint32_t index) const
    {
        return mDisplayViews[index];
    }

    void ClearFrameBuffer();

    void ClearSwapChain();

    void ClearImageView();

    void ClearImage();

private:
    VulkanContextPtr mVulkanContext = nullptr;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;                        //swapchain对象
    uint32_t mSwapchainImageCount = 0;                              //交换链图像的个数

    VkExtent2D mDisplaySize;                                        //图像的大小
    VkFormat mDisplayFormat;                                        //图像的格式

    std::vector<VkImage> mDisplayImages;
    std::vector<VkImageView> mDisplayViews;

    std::vector<VkFramebuffer> mFrameBuffers;

    VkSurfaceCapabilitiesKHR mSurfaceCapabilities;
};

using VulkanSwapChainPtr = std::shared_ptr<VulkanSwapChain>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_SWAPCHAIN_INCLUDE_JDSGHVDHGH */
