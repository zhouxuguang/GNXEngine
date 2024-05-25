//
//  VulkanSwapChain.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanSwapChain.h"

NAMESPACE_RENDERCORE_BEGIN

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
{
    VkExtent2D actualExtent = {width, height};

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

VulkanSwapChain::VulkanSwapChain(VulkanContextPtr vulkanContext, uint32_t width, uint32_t height) : mVulkanContext(vulkanContext)
{
    CreateSwapChain(vulkanContext, width, height);
}

void VulkanSwapChain::CreateSwapChain(VulkanContextPtr vulkanContext, uint32_t width, uint32_t height)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &mSurfaceCapabilities);
    assert(mSurfaceCapabilities.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

    // 查询支持的格式以及选择一个常用的格式如RGBA8
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &formatCount, nullptr);
    
    std::vector<VkSurfaceFormatKHR> formats;
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &formatCount, formats.data());
    
    uint32_t chosenFormat = 0;
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++)
    {
        if (formats[chosenFormat].format == VK_FORMAT_B8G8R8A8_UNORM) break;
    }
    assert(chosenFormat <= formatCount);
    
    mDisplaySize = chooseSwapExtent(mSurfaceCapabilities, width, height);
    mDisplayFormat = formats[chosenFormat].format;
    
    //创建交换链
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = vulkanContext->surfaceKhr;
    swapchainCreateInfo.minImageCount = mSurfaceCapabilities.minImageCount < mSurfaceCapabilities.maxImageCount ?
                            mSurfaceCapabilities.minImageCount + 1 : mSurfaceCapabilities.minImageCount;
    swapchainCreateInfo.imageFormat = formats[chosenFormat].format;
    swapchainCreateInfo.imageColorSpace = formats[chosenFormat].colorSpace;
    swapchainCreateInfo.imageExtent = mDisplaySize;
    swapchainCreateInfo.imageUsage = mSurfaceCapabilities.supportedUsageFlags;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &vulkanContext->graphicsQueueFamilyIndex;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.oldSwapchain = mSwapchain;
    swapchainCreateInfo.clipped = VK_FALSE;
    vkCreateSwapchainKHR(vulkanContext->device, &swapchainCreateInfo, nullptr, &mSwapchain);
    
    // 获得交换链图像
    VkResult res = vkGetSwapchainImagesKHR(vulkanContext->device, mSwapchain, &mSwapchainImageCount, nullptr);
    mDisplayImages.resize(mSwapchainImageCount);
    res = vkGetSwapchainImagesKHR(vulkanContext->device, mSwapchain, &mSwapchainImageCount, mDisplayImages.data());
    
    //为每个交换链图像创建图像视图
    mDisplayViews.resize(mSwapchainImageCount);
    for (uint32_t i = 0; i < mSwapchainImageCount; i++)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.pNext = nullptr;
        viewCreateInfo.image = mDisplayImages[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = mDisplayFormat;
        viewCreateInfo.components =
        {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        };
        viewCreateInfo.subresourceRange =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        viewCreateInfo.flags = 0;
        vkCreateImageView(vulkanContext->device, &viewCreateInfo, nullptr, &mDisplayViews[i]);
    }
}

void VulkanSwapChain::CreateFrameBuffer(VulkanContextPtr vulkanContext, VkRenderPass renderPass, VkImageView depthStencilImage)
{
    // 使用交换链的图像创建帧缓冲
    mFrameBuffers.resize(mSwapchainImageCount);
    for (uint32_t i = 0; i < mSwapchainImageCount; i++)
    {
        std::vector<VkImageView> attachments;
//        if (vulkanContext->numSamples > VK_SAMPLE_COUNT_1_BIT)
//        {
//            attachments.reserve(3);
//            attachments.push_back(_msaaImageView);
//            attachments.push_back(depthStencilImage);
//            attachments.push_back(mDisplayViews[i]);
//        }
//        else
//        {
//            attachments.reserve(2);
//            attachments.push_back(mDisplayViews[i]);
//            attachments.push_back(depthStencilImage);
//        }
        
        attachments.reserve(2);
        attachments.push_back(mDisplayViews[i]);
        attachments.push_back(depthStencilImage);
        
        VkFramebufferCreateInfo fbCreateInfo = {};
        fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCreateInfo.pNext = nullptr;
        fbCreateInfo.flags = 0;
        fbCreateInfo.renderPass = renderPass;
        fbCreateInfo.layers = 1;
        fbCreateInfo.attachmentCount = (uint32_t)attachments.size();
        fbCreateInfo.pAttachments = attachments.data();
        fbCreateInfo.width = uint32_t(mDisplaySize.width);
        fbCreateInfo.height = uint32_t(mDisplaySize.height);
        
        vkCreateFramebuffer(vulkanContext->device, &fbCreateInfo, nullptr, &mFrameBuffers[i]);
    }
}

void VulkanSwapChain::ClearFrameBuffer()
{
    for (auto & iter : mFrameBuffers)
    {
        if (iter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(mVulkanContext->device, iter, NULL);
        }
    }
    mFrameBuffers.clear();
}

void VulkanSwapChain::ClearSwapChain()
{
    if (mSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(mVulkanContext->device, mSwapchain, NULL);
        mSwapchain = VK_NULL_HANDLE;
    }
}

void VulkanSwapChain::ClearImageView()
{
    for (auto & iter : mDisplayViews)
    {
        if (iter != VK_NULL_HANDLE)
        {
            vkDestroyImageView(mVulkanContext->device, iter, NULL);
        }
    }
    mDisplayViews.clear();
}

void VulkanSwapChain::ClearImage()
{
    mDisplayViews.clear();
}

VulkanSwapChain::~VulkanSwapChain()
{
    Release();
}

void VulkanSwapChain::Release()
{
    if (VK_NULL_HANDLE == mVulkanContext->device)
    {
        return;
    }

    ClearFrameBuffer();
    ClearImage();
    ClearImageView();
    ClearSwapChain();
}

NAMESPACE_RENDERCORE_END
