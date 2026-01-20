//
//  VulkanCommandBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VulkanCommandBuffer.h"
#include "VKRenderDevice.h"
#include "VKRenderEncoder.h"
#include "VKComputeEncoder.h"
#include "VKBlitEncoder.h"
#include "VKComputeBuffer.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/DebugBreaker.h"
#include "VKUtil.h"
#include "VKTextureBase.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferInfoPtr commandInfo)
{
    mCommandBuffer = commandBuffer;
    mCommandInfo = commandInfo;
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;
    cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    vkResetCommandBuffer(commandBuffer, 0);

    VkResult res = vkBeginCommandBuffer(mCommandBuffer, &cmdBufferBeginInfo);
    assert(res == VK_SUCCESS);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
}

//创建默认的encoder，也就是屏幕渲染的encoder
RenderEncoderPtr VulkanCommandBuffer::CreateDefaultRenderEncoder() const
{
    VkClearValue clearColor;
    clearColor.color.float32[0] = 0.0;
    clearColor.color.float32[1] = 0.0;
    clearColor.color.float32[2] = 0.0;
    clearColor.color.float32[3] = 1.0;
    
    RenderPassFormat passFormat;
    RenderPassImage passImage;
    RenderPassImageView passImageView;
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(6);
    
    VkImageView imageView = mCommandInfo->swapChain->GetImageView(mCommandInfo->nextFrameIndex);
    
    VkRenderingAttachmentInfoKHR color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	color_attachment_info.imageView = imageView;
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info.clearValue = clearColor;
    clearValues.push_back(clearColor);
    
    VkRenderingAttachmentInfoKHR depth_attachment_info = {};
    depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
    depth_attachment_info.imageView = mCommandInfo->depthStencilBuffer->GetImageView();
    depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_info.clearValue.depthStencil.depth = 1.0;
    depth_attachment_info.clearValue.depthStencil.stencil = 0;
    clearValues.push_back(depth_attachment_info.clearValue);
    clearValues.push_back(depth_attachment_info.clearValue);

    VkRenderingInfoKHR render_info = {};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment_info;
    render_info.pDepthAttachment = &depth_attachment_info;
    render_info.pStencilAttachment = &depth_attachment_info;
    
    render_info.renderArea.offset.x = 0;
    render_info.renderArea.offset.y = 0;
    render_info.renderArea.extent.width = mCommandInfo->swapChain->GetWidth();
    render_info.renderArea.extent.height = mCommandInfo->swapChain->GetHeight();
    
    passFormat.colorFormats.push_back(mCommandInfo->swapChain->GetDisplayFormat());
    passFormat.depthFormat = mCommandInfo->depthStencilBuffer->GetFormat();
    passFormat.stencilFormat = mCommandInfo->depthStencilBuffer->GetFormat();
    
    passImage.colorImages.push_back(mCommandInfo->swapChain->GetImage(mCommandInfo->nextFrameIndex));
    passImage.depthImage = mCommandInfo->depthStencilBuffer->GetImage();
    passImage.stencilImage = mCommandInfo->depthStencilBuffer->GetImage();
    
    passImageView.colorImages.push_back(imageView);
    passImageView.depthImage = mCommandInfo->depthStencilBuffer->GetImageView();
    passImageView.stencilImage = mCommandInfo->depthStencilBuffer->GetImageView();
    
    passImage.isPresentStage = true;
    
    return std::make_shared<VKRenderEncoder>(mCommandInfo->vulkanContext, mCommandBuffer, render_info, 
        passFormat, passImage, clearValues, passImageView, mCommandInfo->currentFrameIndex);
}

RenderEncoderPtr VulkanCommandBuffer::CreateRenderEncoder(const RenderPass& renderPass) const
{
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    
    RenderPassFormat passFormat;
    RenderPassImage passImage;
    RenderPassImageView passImageView;
    
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(6);
    
    for (size_t i = 0; i < renderPass.colorAttachments.size(); i ++)
    {
        RenderPassColorAttachmentPtr iter = renderPass.colorAttachments[i];
        if (!iter)
        {
            continue;
        }
        
        VKTextureBasePtr vkRenderTexture = std::dynamic_pointer_cast<VKTextureBase>(iter->texture);
        
        if (vkRenderTexture == nullptr)
        {
            continue;
        }

        VkImageView imageView = vkRenderTexture->GetImageView()->GetHandle();
        if (vkRenderTexture->GetTextureType() == TextureType_2D_ARRAY || vkRenderTexture->GetTextureType() == TextureType_3D || 
            vkRenderTexture->GetTextureType() == TextureType_CUBE)
        {
            imageView = vkRenderTexture->GetRenderTargetImageView(iter->slice)->GetHandle();
        }
        
        VkClearValue clearColor;
        clearColor.color.float32[0] = iter->clearColor.red;
        clearColor.color.float32[1] = iter->clearColor.green;
        clearColor.color.float32[2] = iter->clearColor.blue;
        clearColor.color.float32[3] = iter->clearColor.alpha;
        clearValues.push_back(clearColor);
        
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageView = imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = GetLoadOP(iter->loadOp);
        colorAttachment.storeOp = GetStoreOP(iter->storeOp);
        colorAttachment.clearValue = clearColor;
        
        colorAttachments.push_back(colorAttachment);
        passFormat.colorFormats.push_back(vkRenderTexture->GetVKFormat());
        passImage.colorImages.push_back(vkRenderTexture->GetVKImage());
        passImageView.colorImages.push_back(imageView);
    }
    
    std::vector<VkRenderingAttachmentInfo> depthAttachments;
    
    if (renderPass.depthAttachment)
    {
        VKTextureBasePtr vkRenderTexture = std::dynamic_pointer_cast<VKTextureBase>(renderPass.depthAttachment->texture);

		VkImageView imageView = vkRenderTexture->GetImageView()->GetHandle();
		if (vkRenderTexture->GetTextureType() == TextureType_2D_ARRAY || vkRenderTexture->GetTextureType() == TextureType_3D ||
			vkRenderTexture->GetTextureType() == TextureType_CUBE)
		{
			imageView = vkRenderTexture->GetRenderTargetImageView(renderPass.depthAttachment->slice)->GetHandle();
		}
        
        VkRenderingAttachmentInfoKHR depthAttachmentInfo = {};
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachmentInfo.imageView = imageView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentInfo.loadOp = GetLoadOP(renderPass.depthAttachment->loadOp);
        depthAttachmentInfo.storeOp = GetStoreOP(renderPass.depthAttachment->storeOp);
        depthAttachmentInfo.clearValue.depthStencil.depth = renderPass.depthAttachment->clearDepth;
        depthAttachments.push_back(depthAttachmentInfo);
        passFormat.depthFormat = vkRenderTexture->GetVKFormat();
        passImage.depthImage = vkRenderTexture->GetVKImage();
        passImageView.depthImage = imageView;
        
        clearValues.push_back(depthAttachmentInfo.clearValue);
    }
   
    // 深度和模板缓冲绑定在一起的格式需要特殊处理
    std::vector<VkRenderingAttachmentInfo> stencilAttachments;
    if (renderPass.stencilAttachment)
    {
        VKTextureBasePtr vkRenderTexture = std::dynamic_pointer_cast<VKTextureBase>(renderPass.stencilAttachment->texture);

		VkImageView imageView = vkRenderTexture->GetImageView()->GetHandle();
		if (vkRenderTexture->GetTextureType() == TextureType_2D_ARRAY || vkRenderTexture->GetTextureType() == TextureType_3D ||
			vkRenderTexture->GetTextureType() == TextureType_CUBE)
		{
			imageView = vkRenderTexture->GetRenderTargetImageView(renderPass.depthAttachment->slice)->GetHandle();
		}
        
        VkRenderingAttachmentInfoKHR stencilAttachmentInfo = {};
        stencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        stencilAttachmentInfo.imageView = imageView;
        stencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencilAttachmentInfo.loadOp = GetLoadOP(renderPass.stencilAttachment->loadOp);
        stencilAttachmentInfo.storeOp = GetStoreOP(renderPass.stencilAttachment->storeOp);
        stencilAttachmentInfo.clearValue.depthStencil.stencil = renderPass.stencilAttachment->clearStencil;;
        stencilAttachments.push_back(stencilAttachmentInfo);
        passFormat.stencilFormat = vkRenderTexture->GetVKFormat();
        passImage.stencilImage = vkRenderTexture->GetVKImage();
        passImageView.stencilImage = imageView;
        
        clearValues.push_back(stencilAttachmentInfo.clearValue);
    }
    
    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = depthAttachments.data();
    renderingInfo.pStencilAttachment = stencilAttachments.data();

    renderingInfo.renderArea.offset.x = renderPass.renderRegion.offsetX;
    renderingInfo.renderArea.offset.y = renderPass.renderRegion.offsetY;
    renderingInfo.renderArea.extent.width = renderPass.renderRegion.width;
    renderingInfo.renderArea.extent.height = renderPass.renderRegion.height;
    
    passImage.isPresentStage = false;
    
    return std::make_shared<VKRenderEncoder>(mCommandInfo->vulkanContext, mCommandBuffer, renderingInfo, passFormat, passImage, 
        clearValues, passImageView, mCommandInfo->currentFrameIndex);
}

ComputeEncoderPtr VulkanCommandBuffer::CreateComputeEncoder() const
{
    return std::make_shared<VKComputeEncoder>(mCommandInfo->vulkanContext, mCommandBuffer);
}

BlitEncoderPtr VulkanCommandBuffer::CreateBlitEncoder() const
{
    return std::make_shared<VKBlitEncoder>(mCommandInfo->vulkanContext, mCommandBuffer);
}

//呈现到屏幕上，上屏
void VulkanCommandBuffer::PresentFrameBuffer()
{
    //结束commandbuffer
    VkResult res = vkEndCommandBuffer(mCommandBuffer);
    
    // 准备等待的信号量
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    
    // 始终等待图像可用的信号量
    waitSemaphores.push_back(mCommandInfo->imageAvailableSemaphore);
    waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    
	// 如果有异步计算工作，等待计算完成
	/*if (mCommandInfo->vulkanContext->asyncComputeSemaphore != VK_NULL_HANDLE)
	{
		waitSemaphores.push_back(mCommandInfo->vulkanContext->asyncComputeSemaphore);
		waitStages.push_back(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}*/
    
    // 信号图形完成
    VkSemaphore signalSemaphores[] = {mCommandInfo->renderFinishSemaphore};
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    res = vkQueueSubmit(mCommandInfo->vulkanContext->graphicsQueue, 1, &submitInfo, mCommandInfo->flightFence);
    
    if (res != VK_SUCCESS)
    {
        LOG_INFO("VulkanCommandBuffer::presentFrameBuffer vkQueueSubmit = %d", res);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            mCommandInfo->renderDevice->Resize(mCommandInfo->swapChain->GetWidth(), mCommandInfo->swapChain->GetHeight());
        }
        else
        {
            return;
        }
    }

    assert(res == VK_SUCCESS);

    //呈现到窗口系统
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapChain = mCommandInfo->swapChain->GetSwapChain();
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &mCommandInfo->nextFrameIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    res = vkQueuePresentKHR(mCommandInfo->vulkanContext->graphicsQueue, &presentInfo);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    {
        char szBuf[32] = {0};
        snprintf(szBuf, 32, "%d", (int)res);
        LOG_INFO("VulkanCommandBuffer::presentFrameBuffer vkQueuePresentKHR = %s", szBuf);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            mCommandInfo->renderDevice->Resize(mCommandInfo->swapChain->GetWidth(), mCommandInfo->swapChain->GetHeight());
        }
        else
        {
            return;
        }
    }
    
    if (res != VK_SUCCESS)
    {
        //baselib::DebugBreak();
    }
    
    // 更新当前帧的索引
    mCommandInfo->renderDevice->UpdateCurrentIndex();
}

//等待命令缓冲区执行完成
void VulkanCommandBuffer::WaitUntilCompleted()
{
    // 从 FencePool 获取 Fence
    VulkanFencePtr fence = mCommandInfo->vulkanContext->fencePool.createFence(mCommandInfo->vulkanContext->device);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;

    vkQueueSubmit(mCommandInfo->vulkanContext->graphicsQueue, 1, &submitInfo, fence->getHandle());

    // 等待 Fence 信号
    fence->wait(mCommandInfo->vulkanContext->device, UINT64_MAX);

    // 释放 Fence
    mCommandInfo->vulkanContext->fencePool.releaseFence(mCommandInfo->vulkanContext->device, fence);
}

//提交命令缓冲区（用于计算命令缓冲区）
void VulkanCommandBuffer::Submit()
{
    //结束commandbuffer
    VkResult res = vkEndCommandBuffer(mCommandBuffer);
    if (res != VK_SUCCESS)
    {
        LOG_INFO("VulkanCommandBuffer::Submit vkEndCommandBuffer failed with error: %d", res);
        return;
    }
    
    // 根据命令缓冲区类型选择队列
    VkQueue submitQueue = mCommandInfo->isComputeCommandBuffer ? 
                        mCommandInfo->vulkanContext->availableComputeQueues[0] : 
                        mCommandInfo->vulkanContext->graphicsQueue;
    
    // 准备同步信息
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> signalSemaphores;
    
    if (mCommandInfo->isComputeCommandBuffer)
    {
        // 计算命令缓冲区：不等待任何东西，直接发出计算完成信号
        // 图形命令缓冲区会在PresentFrameBuffer中等待这个信号
        if (mCommandInfo->vulkanContext->asyncComputeSemaphore != VK_NULL_HANDLE)
        {
            signalSemaphores.push_back(mCommandInfo->vulkanContext->asyncComputeSemaphore);
        }
    }
    else
    {
        // 图形命令缓冲区（在PresentFrameBuffer中使用）
        // 等待计算队列完成（如果有计算工作且信号量存在）
        if (mCommandInfo->vulkanContext->asyncComputeSemaphore != VK_NULL_HANDLE)
        {
            waitSemaphores.push_back(mCommandInfo->vulkanContext->asyncComputeSemaphore);
            waitStages.push_back(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
        
        // 信号图形完成
        signalSemaphores.push_back(mCommandInfo->renderFinishSemaphore);
    }
    
    // 创建提交信息
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();
    
	// 提交到队列
	VkFence submitFence = mCommandInfo->isComputeCommandBuffer ? VK_NULL_HANDLE : mCommandInfo->flightFence;
	res = vkQueueSubmit(submitQueue, 1, &submitInfo, submitFence);
	if (res != VK_SUCCESS)
	{
		LOG_INFO("VulkanCommandBuffer::Submit vkQueueSubmit failed with error: %d", res);
	}
}

void VulkanCommandBuffer::BeginDebugGroup(const char* name, const float color[4])
{
    if (mCommandInfo->vulkanContext->vulkanExtension.enableDebugUtils)
    {
        VkDebugUtilsLabelEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        markerInfo.pLabelName = name;
        memcpy(markerInfo.color, color, sizeof(float) * 4);
        vkCmdBeginDebugUtilsLabelEXT(mCommandBuffer, &markerInfo);
    }
}

void VulkanCommandBuffer::EndDebugGroup()
{
    if (mCommandInfo->vulkanContext->vulkanExtension.enableDebugUtils)
    {
        vkCmdEndDebugUtilsLabelEXT(mCommandBuffer);
    }
}

void VulkanCommandBuffer::ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType)
{
    if (!texture)
    {
        return;
    }

    // 转换到Vulkan纹理
    VKTextureBasePtr vkTexture = std::dynamic_pointer_cast<VKTextureBase>(texture);
    if (!vkTexture)
    {
        return;
    }

    // 确定目标layout
    VkImageLayout targetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags dstAccessMask = 0;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (accessType == ResourceAccessType::ColorAttachment)
    {
        targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (accessType == ResourceAccessType::DepthStencilAttachment)
    {
        targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    else if (accessType == ResourceAccessType::ShaderRead)
    {
        targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (accessType == ResourceAccessType::TransferSrc)
    {
        targetLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (accessType == ResourceAccessType::TransferDst)
    {
        targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        // 未知访问类型，使用general layout
        targetLayout = VK_IMAGE_LAYOUT_GENERAL;
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }

    // 获取当前layout（默认为UNDEFINED）
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // 创建barrier
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = targetLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkTexture->GetVKImage();
    barrier.subresourceRange.aspectMask = VulkanBufferUtil::GetImageAspectFlags(vkTexture->GetVKFormat());

    // 根据纹理格式确定aspect mask
   /* if (VulkanBufferUtil::IsDepthStencilFormat(vkTexture->GetVKFormat()))
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (vkTexture->GetVKFormat() == VK_FORMAT_D24_UNORM_S8_UINT ||
            vkTexture->GetVKFormat() == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }*/

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = vkTexture->GetMipLevels();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = vkTexture->GetLayerCount();

    // 如果当前layout和目标layout相同，不需要转换
    if (currentLayout == targetLayout)
    {
        return;
    }

    // 源access mask（简化处理）
    VkAccessFlags srcAccessMask = 0;
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (currentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    // 插入pipeline barrier
    vkCmdPipelineBarrier(mCommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCommandBuffer::ResourceBarrier(ComputeBufferPtr buffer, ResourceAccessType accessType)
{
    if (!buffer)
    {
        return;
    }

    // 转换到Vulkan计算缓冲区
    VKComputeBuffer* vkBuffer = dynamic_cast<VKComputeBuffer*>(buffer.get());
    if (!vkBuffer)
    {
        return;
    }

    // 确定目标访问掩码和管道阶段
    VkAccessFlags dstAccessMask = 0;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (accessType == ResourceAccessType::ShaderRead)
    {
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
	else if (accessType == ResourceAccessType::ComputeShaderRead)
	{
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
    else if (accessType == ResourceAccessType::ComputeShaderWrite)
    {
        dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (accessType == ResourceAccessType::TransferSrc)
    {
        dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (accessType == ResourceAccessType::TransferDst)
    {
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        // 未知访问类型，默认为shader读写
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }

    // 创建buffer barrier
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = vkBuffer->GetBuffer();
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;

    // 源access mask（简化处理）
    VkAccessFlags srcAccessMask = 0;
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    // 假设之前的状态是transfer write
    if ((accessType & ResourceAccessType::ShaderRead) == ResourceAccessType::ShaderRead)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if ((accessType & ResourceAccessType::ComputeShaderWrite) == ResourceAccessType::ComputeShaderWrite)
    {
        srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    // 插入pipeline barrier
    vkCmdPipelineBarrier(mCommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

NAMESPACE_RENDERCORE_END
