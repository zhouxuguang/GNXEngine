//
//  VKRenderEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKRenderEncoder.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

VkPrimitiveTopology ConvertToVulkanPrimitiveTopology(PrimitiveMode mode)
{
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    switch (mode)
    {
        case PrimitiveMode_POINTS:
            topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            break;
            
        case PrimitiveMode_LINES:
            topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            break;
            
        case PrimitiveMode_LINE_STRIP:
            topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            break;
            
        case PrimitiveMode_TRIANGLES:
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
            
        case PrimitiveMode_TRIANGLE_STRIP:
            topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            break;
            
        default:
            break;
    }
    
    return topology;
}

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

void VKRenderEncoder::setVertexUniformBuffer(UniformBufferPtr buffer, int index)
{
    //
}

void VKRenderEncoder::setFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    //
}

void VKRenderEncoder::drawPrimitves(PrimitiveMode mode, int offset, int size)
{
    //设置图元拓扑类型，需要使用扩展动态状态
#if 0
    // Provided by VK_VERSION_1_3
    void vkCmdSetPrimitiveTopology(
        VkCommandBuffer                             commandBuffer,
        VkPrimitiveTopology                         primitiveTopology);
    
    // Provided by VK_EXT_extended_dynamic_state, VK_EXT_shader_object
    void vkCmdSetPrimitiveTopologyEXT(
        VkCommandBuffer                             commandBuffer,
        VkPrimitiveTopology                         primitiveTopology);
#endif
    vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    
#if 0
    // Provided by VK_VERSION_1_0
    void vkCmdDraw(
        VkCommandBuffer                             commandBuffer,
        uint32_t                                    vertexCount,
        uint32_t                                    instanceCount,
        uint32_t                                    firstVertex,
        uint32_t                                    firstInstance);
#endif
    
    vkCmdDraw(mCommandBuffer, size, 1, offset, 0);
}

void VKRenderEncoder::drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset)
{
    VKIndexBuffer *indexBuffer = (VKIndexBuffer*)buffer.get();
    if (!indexBuffer)
    {
        return;
    }
    
#if 0
    // Provided by VK_VERSION_1_0
    void vkCmdBindIndexBuffer(
        VkCommandBuffer                             commandBuffer,
        VkBuffer                                    buffer,
        VkDeviceSize                                offset,
        VkIndexType                                 indexType);
#endif
    
    // vkCmdBindIndexBuffer 的 offset is the starting offset in bytes within buffer used in index buffer address calculations.
    
    IndexType type = indexBuffer->getIndexType();
    
    int byteOffset = offset * sizeof(uint16_t);
    VkIndexType indexType = VK_INDEX_TYPE_UINT16;
    if (type == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
        indexType = VK_INDEX_TYPE_UINT32;
    }
    
#if 0
    // Provided by VK_VERSION_1_0
    void vkCmdDrawIndexed(
        VkCommandBuffer                             commandBuffer,
        uint32_t                                    indexCount,
        uint32_t                                    instanceCount,
        uint32_t                                    firstIndex,
        int32_t                                     vertexOffset,
        uint32_t                                    firstInstance);
#endif
    
    vkCmdBindIndexBuffer(mCommandBuffer, indexBuffer->GetBuffer(), 0, indexType);
    vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    vkCmdDrawIndexed(mCommandBuffer, size, 1, offset, 0, 0);
}

void VKRenderEncoder::setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index)
{
    //
}

void VKRenderEncoder::setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index)
{
    //
}

void VKRenderEncoder::setFragmentRenderTextureAndSampler(RenderTexturePtr textureCube, TextureSamplerPtr sampler, int index)
{
    //
}

NAMESPACE_RENDERCORE_END
