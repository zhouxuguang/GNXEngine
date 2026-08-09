//
//  VulkanSwapChain.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#include "VulkanSwapChain.h"
#include "Runtime/BaseLib/include/LogService.h"

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
    mDSBuffer = std::make_shared<VKDepthStencilBuffer>(mVulkanContext);
    CreateSwapChain(vulkanContext, width, height);
}

void VulkanSwapChain::CreateSwapChain(VulkanContextPtr vulkanContext, uint32_t width, uint32_t height, bool vSync)
{
    /*if (mDisplaySize.width == width && mDisplaySize.height == height)
    {
        return;
    }*/

    mVSync = vSync;

    //The Vulkan spec states : compositeAlpha must be one of the bits present in the supportedCompositeAlpha member of the 
    // VkSurfaceCapabilitiesKHR structure returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &mSurfaceCapabilities);
    mCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if ((mSurfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR))
	{
		mCompositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}

    // 查询支持的格式以及选择一个常用的格式如RGBA8
    uint32_t formatCount = 0;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &formatCount, nullptr);
    
    std::vector<VkSurfaceFormatKHR> formats;
    formats.resize(formatCount);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &formatCount, formats.data());
    
    uint32_t chosenFormat = 0;
    // 优先选择常见的 B8G8R8A8_UNORM，找不到则用第一个可用格式
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++)
    {
        if (formats[chosenFormat].format == VK_FORMAT_B8G8R8A8_UNORM) break;
    }
    if (chosenFormat >= formatCount && formatCount > 0)
    {
        chosenFormat = 0;  // 回退到第一个可用格式
    }
    assert(formatCount > 0 && chosenFormat < formatCount);
    
    mDisplaySize = chooseSwapExtent(mSurfaceCapabilities, width, height);
    mDisplayFormat = formats[chosenFormat].format;

    // 查询支持的 present mode 并选择合适的模式
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanContext->physicalDevice, vulkanContext->surfaceKhr, &presentModeCount, presentModes.data());

    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;  // 默认 VSync
    if (!vSync)
    {
        // 关闭 VSync：优先 IMMEDIATE，其次 MAILBOX
        if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end())
            selectedPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end())
            selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        // 都不支持则回退到 FIFO（VSync）
    }

    //创建交换链
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = vulkanContext->surfaceKhr;

    // 规范要求 minImageCount 必须在 [minImageCount, maxImageCount] 范围内
    // maxImageCount == 0 表示无上限
    uint32_t minImageCount = mSurfaceCapabilities.minImageCount;
    if (mSurfaceCapabilities.maxImageCount > 0 &&
        minImageCount + 1 > mSurfaceCapabilities.maxImageCount)
    {
        minImageCount = mSurfaceCapabilities.maxImageCount;
    }
    else if (mSurfaceCapabilities.maxImageCount == 0 ||
             minImageCount + 1 <= mSurfaceCapabilities.maxImageCount)
    {
        minImageCount = minImageCount + 1;  // 双缓冲
    }
    swapchainCreateInfo.minImageCount = minImageCount;

    swapchainCreateInfo.imageFormat = formats[chosenFormat].format;
    swapchainCreateInfo.imageColorSpace = formats[chosenFormat].colorSpace;
    swapchainCreateInfo.imageExtent = mDisplaySize;

    // 使用安全的基础 usage：渲染目标 + 可选采样/传输
    // 不要直接用 supportedUsageFlags（某些组合驱动不支持，导致 INITIALIZATION_FAILED）
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (mSurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (mSurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // 必须用 currentTransform（Android/小米设备旋转时非 IDENTITY，硬编码 IDENTITY 会失败）
    swapchainCreateInfo.preTransform = mSurfaceCapabilities.currentTransform;

    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &vulkanContext->graphicsQueueFamilyIndex;
    swapchainCreateInfo.compositeAlpha = mCompositeAlpha;
    swapchainCreateInfo.presentMode = selectedPresentMode;
    swapchainCreateInfo.oldSwapchain = mSwapchain;
    swapchainCreateInfo.clipped = VK_TRUE;
    res = vkCreateSwapchainKHR(vulkanContext->device, &swapchainCreateInfo, nullptr, &mSwapchain);

    if (res != VK_SUCCESS)
    {
        char szBuf[64] = {0};
        snprintf(szBuf, 64, "%d (0x%x)", (int)res, (uint32_t)res);
        LOG_INFO("VulkanSwapChain: vkCreateSwapchainKHR failed: %s", szBuf);
    }
    
    // 获得交换链图像
    res = vkGetSwapchainImagesKHR(vulkanContext->device, mSwapchain, &mSwapchainImageCount, nullptr);
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
        viewCreateInfo.components = {};
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        viewCreateInfo.subresourceRange = {};
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

        viewCreateInfo.flags = 0;
        vkCreateImageView(vulkanContext->device, &viewCreateInfo, nullptr, &mDisplayViews[i]);
    }

    mDSBuffer->CreateDepthStencilBuffer(mVulkanContext, mDisplaySize.width, mDisplaySize.height);
    
}

void VulkanSwapChain::CreateFrameBuffer(VulkanContextPtr vulkanContext, VkRenderPass renderPass, VkImageView depthStencilImage)
{
    // 使用交换链的图像创建帧缓冲，这里帧缓冲区常驻内存
    mFrameBuffers.resize(mSwapchainImageCount);
    for (uint32_t i = 0; i < mSwapchainImageCount; i++)
    {
        std::vector<VkImageView> attachments;
        
        attachments.reserve(3);
        attachments.push_back(mDisplayViews[i]);
        attachments.push_back(depthStencilImage);
        
        VkFramebufferCreateInfo fbCreateInfo = {};
        fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
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
