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

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferInfoPtr commandInfo)
{
    mCommandBuffer = commandBuffer;
    mCommandInfo = commandInfo;
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;
    cmdBufferBeginInfo.flags = 0;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult res = vkBeginCommandBuffer(mCommandBuffer, &cmdBufferBeginInfo);
    assert(res == VK_SUCCESS);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
}

//创建默认的encoder，也就是屏幕渲染的encoder
RenderEncoderPtr VulkanCommandBuffer::createDefaultRenderEncoder() const
{
    VkClearValue clearColor;
    clearColor.color.float32[0] = 0.0;
    clearColor.color.float32[1] = 0.0;
    clearColor.color.float32[2] = 0.0;
    clearColor.color.float32[3] = 1.0;
    
    VkImageView imageView = mCommandInfo->swapChain->GetImageView(mCommandInfo->nextFrameIndex);
    
    const VkRenderingAttachmentInfoKHR color_attachment_info 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };

    VkRenderingInfoKHR render_info 
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        //.renderArea = render_area,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
    };
    
    render_info.renderArea.offset.x = 0;
    render_info.renderArea.offset.y = 0;
    render_info.renderArea.extent.width = mCommandInfo->swapChain->GetWidth();
    render_info.renderArea.extent.height = mCommandInfo->swapChain->GetHeight();
    
    return std::make_shared<VKRenderEncoder>(mCommandBuffer, render_info);
}

RenderEncoderPtr VulkanCommandBuffer::createRenderEncoder(const RenderPass& renderPass) const
{
    return nullptr;
}

ComputeEncoderPtr VulkanCommandBuffer::createComputeEncoder() const
{
    return std::make_shared<VKComputeEncoder>(nullptr, mCommandBuffer);
}

//呈现到屏幕上，上屏
void VulkanCommandBuffer::presentFrameBuffer()
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

    vkResetFences(mCommandInfo->vulkanContext->device, 1, &mCommandInfo->flightFence);
    res = vkQueueSubmit(mCommandInfo->vulkanContext->graphicsQueue, 1, &submitInfo, mCommandInfo->flightFence);
    
    if (res != VK_SUCCESS)
    {
        log_info("VulkanCommandBuffer::presentFrameBuffer vkQueueSubmit = %d", res);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            mCommandInfo->renderDevice->resize(mCommandInfo->swapChain->GetWidth(), mCommandInfo->swapChain->GetHeight());
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
        log_info("VulkanCommandBuffer::presentFrameBuffer vkQueuePresentKHR = %s", szBuf);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
            mCommandInfo->renderDevice->resize(mCommandInfo->swapChain->GetWidth(), mCommandInfo->swapChain->GetHeight());
        }
        else
        {
            return;
        }
    }
    
    assert(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);
    
    // 更新当前帧的索引
    mCommandInfo->renderDevice->UpdateCurrentIndex();
}

//等待命令缓冲区执行完成
void VulkanCommandBuffer::waitUntilCompleted()
{
    //
}

NAMESPACE_RENDERCORE_END
