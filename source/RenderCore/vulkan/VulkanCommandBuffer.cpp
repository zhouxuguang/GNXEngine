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
#include "BaseLib/LogService.h"
#include "BaseLib/DebugBreaker.h"
#include "VKUtil.h"
#include "VKTextureBase.h"

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
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
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
    
    uint32_t width = 0;
    uint32_t height = 0;
    
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
        
        VkClearValue clearColor;
        clearColor.color.float32[0] = iter->clearColor.red;
        clearColor.color.float32[1] = iter->clearColor.green;
        clearColor.color.float32[2] = iter->clearColor.blue;
        clearColor.color.float32[3] = iter->clearColor.alpha;
        clearValues.push_back(clearColor);
        
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageView = vkRenderTexture->GetImageView()->GetHandle();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = GetLoadOP(iter->loadOp);
        colorAttachment.storeOp = GetStoreOP(iter->storeOp);
        colorAttachment.clearValue = clearColor;
        
        width = vkRenderTexture->GetWidth();
        height = vkRenderTexture->GetHeight();
        
        colorAttachments.push_back(colorAttachment);
        passFormat.colorFormats.push_back(vkRenderTexture->GetVKFormat());
        passImage.colorImages.push_back(vkRenderTexture->GetVKImage());
        passImageView.colorImages.push_back(vkRenderTexture->GetImageView()->GetHandle());
    }
    
    std::vector<VkRenderingAttachmentInfo> depthAttachments;
    
    if (renderPass.depthAttachment)
    {
        VKTextureBasePtr vkRenderTexture = std::dynamic_pointer_cast<VKTextureBase>(renderPass.depthAttachment->texture);
        
        VkRenderingAttachmentInfoKHR depth_attachment_info = {};
        depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depth_attachment_info.imageView = vkRenderTexture->GetImageView()->GetHandle();
        depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment_info.loadOp = GetLoadOP(renderPass.depthAttachment->loadOp);
        depth_attachment_info.storeOp = GetStoreOP(renderPass.depthAttachment->storeOp);
        depth_attachment_info.clearValue.depthStencil.depth = renderPass.depthAttachment->clearDepth;
        depth_attachment_info.clearValue.depthStencil.stencil = renderPass.stencilAttachment->clearStencil;
        depthAttachments.push_back(depth_attachment_info);
        passFormat.depthFormat = vkRenderTexture->GetVKFormat();
        passImage.depthImage = vkRenderTexture->GetVKImage();
        passImageView.depthImage = vkRenderTexture->GetImageView()->GetHandle();
        
        clearValues.push_back(depth_attachment_info.clearValue);
    }
   
    // 深度和模板缓冲绑定在一起的格式需要特殊处理
    std::vector<VkRenderingAttachmentInfo> stencilAttachments;
    if (renderPass.stencilAttachment)
    {
        VKTextureBasePtr vkRenderTexture = std::dynamic_pointer_cast<VKTextureBase>(renderPass.stencilAttachment->texture);
        
        VkRenderingAttachmentInfoKHR stencil_attachment_info = {};
        stencil_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        stencil_attachment_info.imageView = vkRenderTexture->GetImageView()->GetHandle();
        stencil_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencil_attachment_info.loadOp = GetLoadOP(renderPass.stencilAttachment->loadOp);
        stencil_attachment_info.storeOp = GetStoreOP(renderPass.stencilAttachment->storeOp);
        stencil_attachment_info.clearValue.depthStencil.stencil = 0x0;
        stencilAttachments.push_back(stencil_attachment_info);
        passFormat.stencilFormat = vkRenderTexture->GetVKFormat();
        passImage.stencilImage = vkRenderTexture->GetVKImage();
        passImageView.stencilImage = vkRenderTexture->GetImageView()->GetHandle();
        
        clearValues.push_back(stencil_attachment_info.clearValue);
    }
    
    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = depthAttachments.data();
    renderingInfo.pStencilAttachment = stencilAttachments.data();
    renderingInfo.renderArea.extent.width = width;
    renderingInfo.renderArea.extent.height = height;
    
    passImage.isPresentStage = false;
    
    return std::make_shared<VKRenderEncoder>(mCommandInfo->vulkanContext, mCommandBuffer, renderingInfo, passFormat, passImage, 
        clearValues, passImageView, mCommandInfo->currentFrameIndex);
}

ComputeEncoderPtr VulkanCommandBuffer::CreateComputeEncoder() const
{
    return std::make_shared<VKComputeEncoder>(mCommandInfo->vulkanContext, mCommandBuffer);
}

//呈现到屏幕上，上屏
void VulkanCommandBuffer::PresentFrameBuffer()
{
    //结束commandbuffer
    VkResult res = vkEndCommandBuffer(mCommandBuffer);
    
    //提交渲染命令到队列
    VkSemaphore waitSemaphores[] = {mCommandInfo->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
    VkSemaphore signalSemaphores[] = {mCommandInfo->renderFinishSemaphore};
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //vkResetFences(mCommandInfo->vulkanContext->device, 1, &mCommandInfo->flightFence);
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
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    
    vkQueueSubmit(mCommandInfo->vulkanContext->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mCommandInfo->vulkanContext->graphicsQueue);
}

NAMESPACE_RENDERCORE_END
