//
//  VKRenderEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKRenderEncoder.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKUniformBuffer.h"
#include "VKTexture2D.h"
#include "VKTextureSampler.h"
#include "VKTextureCube.h"
#include "VKRenderTexture.h"
#include "VKGraphicsPipeline.h"
#include "VulkanDescriptorUtil.h"
#include "VulkanBufferUtil.h"

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

VKRenderEncoder::VKRenderEncoder(VkCommandBuffer commandBuffer, const VkRenderingInfoKHR& renderInfo, 
                                 const RenderPassFormat& passFormat, const RenderPassImage& passImage)
    : mPassFormat(passFormat), mPassImage(passImage)
{
    mCommandBuffer = commandBuffer;
    
    // 动态渲染没有子流程依赖，所以需要插入图像内存屏障
    for (auto &iter : mPassImage.colorImages)
    {
        VulkanBufferUtil::InsertImageMemoryBarrier(
                                                   mCommandBuffer,
                                                   iter,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    }
    
    if (mPassImage.depthImage)
    {
        VulkanBufferUtil::InsertImageMemoryBarrier(
                                                   mCommandBuffer,
                                                   mPassImage.depthImage,
            0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });
    }
    
    if (mPassImage.stencilImage)
    {
        VulkanBufferUtil::InsertImageMemoryBarrier(
                                                   mCommandBuffer,
                                                   mPassImage.stencilImage,
            0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });
    }
    
    vkCmdBeginRenderingKHR(mCommandBuffer, &renderInfo);
    
    //设置viewport
    VkViewport viewport;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    // 设置负的高度
    viewport.x = renderInfo.renderArea.offset.x;
    viewport.y = (float)renderInfo.renderArea.extent.height - renderInfo.renderArea.offset.y;
    viewport.width = (float)renderInfo.renderArea.extent.width;
    // [POI] Flip the sign of the viewport's height
    viewport.height = -(float)renderInfo.renderArea.extent.height;
    
    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);

    //设置裁剪区域
    vkCmdSetScissor(mCommandBuffer, 0, 1, &renderInfo.renderArea);
}

VKRenderEncoder::~VKRenderEncoder()
{
    //vkResetCommandBuffer(mCommandBuffer, 0);
}

void VKRenderEncoder::EndEncode()
{
    vkCmdEndRenderingKHR(mCommandBuffer);
    
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (!mPassImage.isPresentStage)
    {
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    // 对颜色附件进行转换
    for (auto &iter : mPassImage.colorImages)
    {
        VulkanBufferUtil::InsertImageMemoryBarrier(
                                             mCommandBuffer,
                                                   iter,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                   imageLayout,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    }
}

void VKRenderEncoder::setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
{
    if (!graphicsPipeline)
    {
        return;
    }
    
    VKGraphicsPipeline *vkGraphicsPipieline = (VKGraphicsPipeline *)graphicsPipeline.get();
    
    vkGraphicsPipieline->Generate(mPassFormat);
    mGraphicsPipieline = vkGraphicsPipieline;
    
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipieline->GetPipeline());
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
    if (!buffer)
    {
        return;
    }
    VKUniformBuffer *vkUniformBuffer = (VKUniformBuffer*)buffer.get();
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfo.buffer = vkUniformBuffer->GetBuffer();
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferInfo);   // 这里还有问题，索引binding的问题
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Vertex, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, &writeDescriptorSet);
}

void VKRenderEncoder::setFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }
    VKUniformBuffer *vkUniformBuffer = (VKUniformBuffer*)buffer.get();
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfo.buffer = vkUniformBuffer->GetBuffer();
    
    // 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, index, &bufferInfo);
    writeDescriptorSet.dstSet = 0;   //这句也是可以的
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, &writeDescriptorSet);
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
    if (!texture || !sampler)
    {
        return;
    }
    VKTexture2D *vkTexture2D = (VKTexture2D*)texture.get();
    VKTextureSampler* vkSampler = (VKTextureSampler*)sampler.get();
    
    //VkWriteDescriptorSet
    VkWriteDescriptorSet writeDesSets[2];
    
    VkDescriptorImageInfo imageInfo = VulkanDescriptorUtil::DescriptorImageInfo(0,
                        vkTexture2D->getVKImageView()->GetHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[0] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, &imageInfo);
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, writeDesSets);
    
    VkDescriptorImageInfo samplerInfo = VulkanDescriptorUtil::DescriptorImageInfo(vkSampler->GetVKSampler(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[1] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLER, index, &samplerInfo);
    
    uint32_t samOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLER);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), samOffset, 1, writeDesSets + 1);
}

void VKRenderEncoder::setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index)
{
    if (!textureCube || !sampler)
    {
        return;
    }
    VKTextureCube *vkTextureCube = (VKTextureCube*)textureCube.get();
    VKTextureSampler* vkSampler = (VKTextureSampler*)sampler.get();
    
    VkWriteDescriptorSet writeDesSets[2];
    
    VkDescriptorImageInfo imageInfo = VulkanDescriptorUtil::DescriptorImageInfo(0, vkTextureCube->GetImageView()->GetHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[0] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, &imageInfo);
    
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, writeDesSets);
    
    VkDescriptorImageInfo samplerInfo = VulkanDescriptorUtil::DescriptorImageInfo(vkSampler->GetVKSampler(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[1] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLER, index, &samplerInfo);
    
    uint32_t samOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLER);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), samOffset, 1, writeDesSets + 1);
}

void VKRenderEncoder::setFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index)
{
    if (!renderTexture || !sampler)
    {
        return;
    }
    VKRenderTexture *vkTexture = (VKRenderTexture*)renderTexture.get();
    VKTextureSampler* vkSampler = (VKTextureSampler*)sampler.get();
    
    VkWriteDescriptorSet writeDesSets[2];
    
    VkDescriptorImageInfo imageInfo = VulkanDescriptorUtil::DescriptorImageInfo(0, vkTexture->GetImageView()->GetHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[0] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, &imageInfo);
    
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, writeDesSets);
    
    VkDescriptorImageInfo samplerInfo = VulkanDescriptorUtil::DescriptorImageInfo(vkSampler->GetVKSampler(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    writeDesSets[1] = VulkanDescriptorUtil::GetImageWriteDescriptorSet(VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_SAMPLER, index, &samplerInfo);
    
    uint32_t samOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_SAMPLER);
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), samOffset, 1, writeDesSets + 1);
}

NAMESPACE_RENDERCORE_END
