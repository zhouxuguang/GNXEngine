//
//  VKRenderEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKRenderEncoder.h"
#include "VKVertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

VKRenderEncoder::VKRenderEncoder(VkCommandBuffer commandBuffer, const VkRenderingInfoKHR& renderInfo)
{
    mCommandBuffer = commandBuffer;
    vkCmdBeginRenderingKHR(mCommandBuffer, &renderInfo);
    
    //设置viewport
    VkViewport viewport;
    viewport.x = renderInfo.renderArea.offset.x;
    viewport.y = renderInfo.renderArea.offset.y;
    viewport.width = (float)renderInfo.renderArea.extent.width;
    viewport.height = (float)renderInfo.renderArea.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);

    //设置裁剪区域
    vkCmdSetScissor(mCommandBuffer, 0, 1, &renderInfo.renderArea);
}

VKRenderEncoder::~VKRenderEncoder()
{
}

void VKRenderEncoder::EndEncode()
{
    vkCmdEndRenderingKHR(mCommandBuffer);
}

void VKRenderEncoder::setVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index)
{
//    void vkCmdBindVertexBuffers(
//        VkCommandBuffer                             commandBuffer,
//        uint32_t                                    firstBinding,
//        uint32_t                                    bindingCount,
//        const VkBuffer*                             pBuffers,
//        const VkDeviceSize*                         pOffsets);
    if (!buffer)
    {
        return;
    }
    
    VKVertexBuffer* vkBuffer = (VKVertexBuffer*)buffer.get();
    VkBuffer innerBuffer = vkBuffer->GetGpuBuffer();
    VkDeviceSize deviceOffset = offset;
    
    vkCmdBindVertexBuffers(mCommandBuffer, index, 1, &innerBuffer, &deviceOffset);
}

NAMESPACE_RENDERCORE_END
