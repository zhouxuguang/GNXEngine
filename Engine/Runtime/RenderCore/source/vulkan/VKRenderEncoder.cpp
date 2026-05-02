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
#include "VKTextureSampler.h"
#include "VKGraphicsPipeline.h"
#include "VulkanDescriptorUtil.h"
#include "VulkanBufferUtil.h"
#include "VKTextureBase.h"

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

void VKRenderEncoder::BeginDynamicRenderPass(const VkRenderingInfoKHR& renderInfo)
{
    // 动态渲染没有子流程依赖，所以需要插入图像内存屏障
    // 注意：FrameGraph 的 preWrite 已经处理了大部分布局转换，这里只处理未转换的情况
    for (size_t i = 0; i < mPassImage.colorImages.size(); ++i)
    {
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (!mPassTexture.colorTextures.empty() && i < mPassTexture.colorTextures.size() && mPassTexture.colorTextures[i])
        {
            currentLayout = mPassTexture.colorTextures[i]->GetCurrentLayout();
        }

        // 如果布局已经是 COLOR_ATTACHMENT_OPTIMAL，跳过 barrier
        if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            continue;
        }

        VulkanBufferUtil::InsertImageMemoryBarrier(
            mCommandBuffer,
            mPassImage.colorImages[i],
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            currentLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        // 更新纹理layout
        if (!mPassTexture.colorTextures.empty() && i < mPassTexture.colorTextures.size() && mPassTexture.colorTextures[i])
        {
            mPassTexture.colorTextures[i]->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    }

    if (mPassImage.depthImage && !mPassTexture.colorTextures.empty() && mPassTexture.depthTexture)
    {
        VkImageLayout currentLayout = mPassTexture.depthTexture->GetCurrentLayout();
        
        // 从renderInfo获取目标布局（由CreateRenderEncoder设置）
        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAccessFlags dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        if (renderInfo.pDepthAttachment)
        {
            targetLayout = renderInfo.pDepthAttachment->imageLayout;
            // 只读深度附件只需要读取访问权限
            if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
            {
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }

        // 如果布局已经是目标布局，跳过 barrier
        if (currentLayout != targetLayout)
        {
            VulkanBufferUtil::InsertImageMemoryBarrier(
                mCommandBuffer,
                mPassImage.depthImage,
                0,
                dstAccessMask,
                currentLayout,
                targetLayout,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VkImageSubresourceRange{ VulkanBufferUtil::GetImageAspectFlags(mPassFormat.depthFormat), 0, 1, 0, 1 });

            mPassTexture.depthTexture->SetCurrentLayout(targetLayout);
        }
    }
    else if (mPassImage.depthImage)
    {
        // 从renderInfo获取目标布局
        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAccessFlags dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        if (renderInfo.pDepthAttachment)
        {
            targetLayout = renderInfo.pDepthAttachment->imageLayout;
            if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
            {
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }
        
        VulkanBufferUtil::InsertImageMemoryBarrier(
            mCommandBuffer,
            mPassImage.depthImage,
            0,
            dstAccessMask,
            VK_IMAGE_LAYOUT_UNDEFINED,
            targetLayout,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VulkanBufferUtil::GetImageAspectFlags(mPassFormat.depthFormat), 0, 1, 0, 1 });
    }

    if (mPassImage.stencilImage && !mPassTexture.colorTextures.empty() && mPassTexture.stencilTexture)
    {
        VkImageLayout currentLayout = mPassTexture.stencilTexture->GetCurrentLayout();
        
        // 从renderInfo获取目标布局（由CreateRenderEncoder设置）
        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAccessFlags dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        if (renderInfo.pStencilAttachment)
        {
            targetLayout = renderInfo.pStencilAttachment->imageLayout;
            // 只读模板附件只需要读取访问权限
            if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
            {
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }

        // 如果布局已经是目标布局，跳过 barrier
        if (currentLayout != targetLayout)
        {
            VulkanBufferUtil::InsertImageMemoryBarrier(
                mCommandBuffer,
                mPassImage.stencilImage,
                0,
                dstAccessMask,
                currentLayout,
                targetLayout,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VkImageSubresourceRange{ VulkanBufferUtil::GetImageAspectFlags(mPassFormat.stencilFormat), 0, 1, 0, 1 });

            mPassTexture.stencilTexture->SetCurrentLayout(targetLayout);
        }
    }
    else if (mPassImage.stencilImage)
    {
        // 从renderInfo获取目标布局
        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAccessFlags dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        if (renderInfo.pStencilAttachment)
        {
            targetLayout = renderInfo.pStencilAttachment->imageLayout;
            if (targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
            {
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }
        
        VulkanBufferUtil::InsertImageMemoryBarrier(
            mCommandBuffer,
            mPassImage.stencilImage,
            0,
            dstAccessMask,
            VK_IMAGE_LAYOUT_UNDEFINED,
            targetLayout,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VkImageSubresourceRange{ VulkanBufferUtil::GetImageAspectFlags(mPassFormat.stencilFormat), 0, 1, 0, 1 });
    }

    vkCmdBeginRenderingKHR(mCommandBuffer, &renderInfo);
}

void VKRenderEncoder::EndDynamicRenderPass()
{
    vkCmdEndRenderingKHR(mCommandBuffer);

    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (!mPassImage.isPresentStage)
    {
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // 对颜色附件进行转换
    for (size_t i = 0; i < mPassImage.colorImages.size(); ++i)
    {
        VulkanBufferUtil::InsertImageMemoryBarrier(
            mCommandBuffer,
            mPassImage.colorImages[i],
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            imageLayout,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        // 更新纹理layout
        if (!mPassTexture.colorTextures.empty() && i < mPassTexture.colorTextures.size() && mPassTexture.colorTextures[i])
        {
            mPassTexture.colorTextures[i]->SetCurrentLayout(imageLayout);
        }
    }
}

void VKRenderEncoder::BeginRenderPass(const VkRenderingInfoKHR& renderInfo, 
                                      const RenderPassFormat& passFormat,
                                      const RenderPassImage& passImage, 
                                      const std::vector<VkClearValue> &clearValues,
                                      const RenderPassImageView& passImageView)
{
    mRenderPass = std::make_shared<VulkanRenderPass>(mContext, passFormat);
    
    // 创建fb
    std::vector<VkImageView> attachments;
    
    attachments.reserve(6);
    for (const auto & iter : passImageView.colorImages)
    {
        attachments.push_back(iter);
    }
    if (passImageView.depthImage)
    {
        attachments.push_back(passImageView.depthImage);
    }
    if (passImageView.stencilImage)
    {
        attachments.push_back(passImageView.stencilImage);
    }
    
    VkFramebufferCreateInfo fbCreateInfo = {};
    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.renderPass = mRenderPass->GetRenderPass();
    fbCreateInfo.layers = 1;
    fbCreateInfo.attachmentCount = (uint32_t)attachments.size();
    fbCreateInfo.pAttachments = attachments.data();
    fbCreateInfo.width = renderInfo.renderArea.extent.width;
    fbCreateInfo.height = renderInfo.renderArea.extent.height;
    
    vkCreateFramebuffer(mContext->device, &fbCreateInfo, nullptr, &mFrameBuffer);
    
    // Now we start a renderpass. Any draw command has to be recorded in a renderpass
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mRenderPass->GetRenderPass();
    renderPassBeginInfo.framebuffer = mFrameBuffer;   //fb还需要创建
    
    renderPassBeginInfo.renderArea.offset.x = renderInfo.renderArea.offset.x;
    renderPassBeginInfo.renderArea.offset.y = renderInfo.renderArea.offset.y;
    renderPassBeginInfo.renderArea.extent.width = renderInfo.renderArea.extent.width;
    renderPassBeginInfo.renderArea.extent.height = renderInfo.renderArea.extent.height;
            
    renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VKRenderEncoder::EndRenderPass()
{
    //结束renderpass
    vkCmdEndRenderPass(mCommandBuffer);
}

VKRenderEncoder::VKRenderEncoder(VulkanContextPtr context,
                                 VkCommandBuffer commandBuffer,
                                 const VkRenderingInfoKHR& renderInfo,
                                 const RenderPassFormat& passFormat,
                                 const RenderPassImage& passImage,
                                 const RenderPassTexture& passTexture,
                                 const std::vector<VkClearValue> &clearValues,
                                 const RenderPassImageView& passImageView,
                                 uint32_t currentFrameIndex) : mPassFormat(passFormat), mPassImage(passImage), mPassTexture(passTexture)
{
    mCommandBuffer = commandBuffer;
    mContext = context;
    mCurrentFrameIndex = currentFrameIndex;
    
    if (mContext->vulkanExtension.enabledDynamicRendering)
    {
        BeginDynamicRenderPass(renderInfo);
    }
    else
    {
        BeginRenderPass(renderInfo, passFormat, passImage, clearValues, passImageView);
    }
    
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
    
    if (mContext->vulkanExtension.enableDebugUtils)
    {
        VkDebugUtilsLabelEXT debugLabel = {};
        debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        debugLabel.pLabelName = "RenderPass";
        float color[4] = {1.0, 1.0, 1.0, 1.0};
        vkCmdBeginDebugUtilsLabelEXT(mCommandBuffer, &debugLabel);
    }
}

VKRenderEncoder::~VKRenderEncoder()
{
    // 如果 render pass 仍然活跃，自动结束它
    // 这可以防止在 render pass 内部调用 vkCmdPipelineBarrier 导致的验证错误
    if (mIsEncoding)
    {
        EndEncode();
    }
    
    SafeDestroyFramebuffer(*mContext, mFrameBuffer);
}

void VKRenderEncoder::EndEncode()
{
    if (!mIsEncoding)
    {
        return;  // 已经结束，避免重复调用
    }

    mIsEncoding = false;

    if (mContext->vulkanExtension.enabledDynamicRendering)
    {
        EndDynamicRenderPass();
    }
    else
    {
        EndRenderPass();
    }

    // 结束构造函数中开始的 "RenderPass" debug marker
    if (mContext->vulkanExtension.enableDebugUtils)
    {
        vkCmdEndDebugUtilsLabelEXT(mCommandBuffer);
    }
}

void VKRenderEncoder::BindPipeline()
{
    if (!mGraphicsPipieline)
    {
        return;
    }
    if (mGraphicsPipieline && !mGraphicsPipieline->IsGenerated())
    {
        mGraphicsPipieline->Generate(mPassFormat);
    }
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipeline());

	// Provided by VK_VERSION_1_0
	/*void vkCmdBindDescriptorSets(
		VkCommandBuffer                             commandBuffer,
		VkPipelineBindPoint                         pipelineBindPoint,
		VkPipelineLayout                            layout,
		uint32_t                                    firstSet,
		uint32_t                                    descriptorSetCount,
		const VkDescriptorSet * pDescriptorSets,
		uint32_t                                    dynamicOffsetCount,
		const uint32_t * pDynamicOffsets);*/

    if (!mContext->vulkanExtension.enablePushDesDescriptor)
    {
		VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();
		const VKGraphicsShader::OneFrameDescriptorSets& desCriptors = shader->GetCurrentDescriptorSets();

        if (!desCriptors.empty())
        {
			vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				mGraphicsPipieline->GetPipelineLayout(), 0, desCriptors.size(),
				desCriptors.data(), 0, nullptr);
        }
    }
}

void VKRenderEncoder::SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
{
    if (!graphicsPipeline)
    {
        return;
    }
    
    VKGraphicsPipeline *vkGraphicsPipieline = (VKGraphicsPipeline *)graphicsPipeline.get();
    assert(vkGraphicsPipieline);
    mGraphicsPipieline = vkGraphicsPipieline;
    mGraphicsPipieline->SetCurrentFrameIndex(mCurrentFrameIndex);
    
    // 没有动态渲染时需要设置renderpass
    if (!mContext->vulkanExtension.enabledDynamicRendering)
    {
        mGraphicsPipieline->SetRenderPass(mRenderPass->GetRenderPass());
    }
    
    BindPipeline();

    // 当 VK_DYNAMIC_STATE_POLYGON_MODE_EXT 被启用时，静态 polygonMode 被忽略，
    // 必须在绑定管线后动态设置，否则后续绘制会使用未定义的填充模式
    if (mContext->vulkanExtension.enabledExtendedDynamicState3)
    {
        VkPolygonMode polygonMode = (mGraphicsPipieline->GetDesc().fillMode == FillModeWireframe)
            ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        vkCmdSetPolygonModeEXT(mCommandBuffer, polygonMode);
    }
}

void VKRenderEncoder::SetFillMode(FillMode fillMode)
{
    if (mContext->vulkanExtension.enabledExtendedDynamicState3)
    {
        VkPolygonMode polygonMode = (fillMode == FillModeWireframe) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        vkCmdSetPolygonModeEXT(mCommandBuffer, polygonMode);
    }
}

void VKRenderEncoder::SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index)
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

void VKRenderEncoder::SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index)
{
    if (!buffer)
    {
        return;
    }
    
    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (!vkBuffer)
    {
        return;
    }
    
    VkBuffer innerBuffer = vkBuffer->GetVkBuffer();
    VkDeviceSize deviceOffset = offset;
    
    vkCmdBindVertexBuffers(mCommandBuffer, index, 1, &innerBuffer, &deviceOffset);
}

void VKRenderEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage)
{
    if (!buffer || !mGraphicsPipieline)
    {
        return;
    }
    
    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (!vkBuffer)
    {
        return;
    }
    
    uint32_t bindIndex = mGraphicsPipieline->GetResourceBindIndex(resourceName);
    if (bindIndex == (uint32_t)-1)
    {
        return;
    }
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vkBuffer->GetVkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(VK_NULL_HANDLE,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindIndex, &bufferInfo);
    writeDescriptorSet.dstSet = 0;
    
    uint32_t bufSetOffset = mGraphicsPipieline->GetSetOffset(DESCRIPTOR_TYPE_STORAGE_BUFFER);
    
    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        mGraphicsPipieline->GetPipelineLayout(), bufSetOffset, 1, &writeDescriptorSet);
}

void VKRenderEncoder::SetVertexUniformBuffer(UniformBufferPtr buffer, int index)
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
    uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Vertex, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    VkDescriptorSet descriptorSet = mGraphicsPipieline->GetDescriptorSet(texOffset);
    VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(descriptorSet,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, index, &bufferInfo);   // 这里还有问题，索引binding的问题
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = VK_NULL_HANDLE;
    imageInfo.sampler = VK_NULL_HANDLE;
    writeDescriptorSet.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(mContext->device, 1, &writeDescriptorSet, 0, nullptr);
    //vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, &writeDescriptorSet);
}

void VKRenderEncoder::SetFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    //if (!buffer)
    //{
    //    return;
    //}
    //VKUniformBuffer *vkUniformBuffer = (VKUniformBuffer*)buffer.get();
    //
    //VkDescriptorBufferInfo bufferInfo = {};
    //bufferInfo.range = VK_WHOLE_SIZE;
    //bufferInfo.buffer = vkUniformBuffer->GetBuffer();
    //
    //// 注意 使用了 pushDescriptorSet了，VkDescriptorSet就必须设置为空
    //uint32_t texOffset = mGraphicsPipieline->GetDescriptorOffset(ShaderStage_Fragment, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    //VkDescriptorSet descriptorSet = mGraphicsPipieline->GetDescriptorSet(texOffset);
    //VkWriteDescriptorSet writeDescriptorSet = VulkanDescriptorUtil::GetBufferWriteDescriptorSet(descriptorSet,
    //            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, index, &bufferInfo);
    //
    //vkUpdateDescriptorSets(mContext->device, 1, &writeDescriptorSet, 0, nullptr);
    //	//vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipieline->GetPipelineLayout(), texOffset, 1, &writeDescriptorSet);
}

void VKRenderEncoder::SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture)
{
	VKTextureBasePtr vkTexture = std::dynamic_pointer_cast<VKTextureBase>(texture);
    if (!vkTexture)
    {
        return;
    }

	VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();

	ShaderImageDesc imageDesc;
	imageDesc.image = vkTexture->GetImageView()->GetHandle();
	imageDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageDesc.sampler = VK_NULL_HANDLE;

	shader->BindTexture(mCommandBuffer, resourceName, imageDesc, mGraphicsPipieline->GetPipelineLayout());
}

void VKRenderEncoder::SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
	if (!buffer)
	{
		return;
	}
	VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();

    VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();

    ShaderBufferDesc bufferDesc;
    bufferDesc.buffer = vkUniformBuffer->GetBuffer();
    bufferDesc.offset = 0;
    bufferDesc.range = VK_WHOLE_SIZE;

    shader->BindUniformBuffer(mCommandBuffer, resourceName, bufferDesc, mGraphicsPipieline->GetPipelineLayout());
}

void VKRenderEncoder::SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
	if (!buffer)
	{
		return;
	}
	VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();

	VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();

	ShaderBufferDesc bufferDesc;
	bufferDesc.buffer = vkUniformBuffer->GetBuffer();
	bufferDesc.offset = 0;
	bufferDesc.range = VK_WHOLE_SIZE;

	shader->BindUniformBuffer(mCommandBuffer, resourceName, bufferDesc, mGraphicsPipieline->GetPipelineLayout());
}

void VKRenderEncoder::SetMeshUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer || !mGraphicsPipieline)
    {
        return;
    }

    VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vkUniformBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    // Mesh shader 使用 push descriptor 绑定 uniform buffer
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = VK_NULL_HANDLE;
    writeDescriptorSet.dstBinding = index;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &bufferInfo;

    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mGraphicsPipieline->GetPipelineLayout(), 0, 1, &writeDescriptorSet);
}

void VKRenderEncoder::SetTaskUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer || !mGraphicsPipieline)
    {
        return;
    }

    VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vkUniformBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    // Task shader 使用 push descriptor 绑定 uniform buffer
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = VK_NULL_HANDLE;
    writeDescriptorSet.dstBinding = index;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &bufferInfo;

    vkCmdPushDescriptorSetKHR(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mGraphicsPipieline->GetPipelineLayout(), 0, 1, &writeDescriptorSet);
}

void VKRenderEncoder::SetMeshUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    if (!buffer || !mGraphicsPipieline)
    {
        return;
    }

    VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();
    VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();
    if (!shader)
    {
        return;
    }

    ShaderBufferDesc bufferDesc;
    bufferDesc.buffer = vkUniformBuffer->GetBuffer();
    bufferDesc.offset = 0;
    bufferDesc.range = VK_WHOLE_SIZE;

    shader->BindUniformBuffer(mCommandBuffer, resourceName, bufferDesc, mGraphicsPipieline->GetPipelineLayout());
}

void VKRenderEncoder::SetTaskUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    if (!buffer || !mGraphicsPipieline)
    {
        return;
    }

    VKUniformBuffer* vkUniformBuffer = (VKUniformBuffer*)buffer.get();
    VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();
    if (!shader)
    {
        return;
    }

    ShaderBufferDesc bufferDesc;
    bufferDesc.buffer = vkUniformBuffer->GetBuffer();
    bufferDesc.offset = 0;
    bufferDesc.range = VK_WHOLE_SIZE;

    shader->BindUniformBuffer(mCommandBuffer, resourceName, bufferDesc, mGraphicsPipieline->GetPipelineLayout());
}

void VKRenderEncoder::DrawPrimitives(PrimitiveMode mode, int offset, int size)
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
    if (mContext->vulkanExtension.enabledExtendedDynamicState)
    {
        vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    }
    else
    {
        //mGraphicsPipieline->SetPrimitiveType(ConvertToVulkanPrimitiveTopology(mode));
    }
    //BindPipeline();
    
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

void VKRenderEncoder::DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount)
{
	//设置图元拓扑类型，需要使用扩展动态状态
	if (mContext->vulkanExtension.enabledExtendedDynamicState)
	{
		vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
	}
	else
	{
		//mGraphicsPipieline->SetPrimitiveType(ConvertToVulkanPrimitiveTopology(mode));
	}

#if 0
	// Provided by VK_VERSION_1_0
	void vkCmdDraw(
		VkCommandBuffer                             commandBuffer,
		uint32_t                                    vertexCount,
		uint32_t                                    instanceCount,
		uint32_t                                    firstVertex,
		uint32_t                                    firstInstance);
#endif

	vkCmdDraw(mCommandBuffer, size, instanceCount, offset, firstInstance);
}

void VKRenderEncoder::DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex)
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
    if (mContext->vulkanExtension.enabledExtendedDynamicState)
    {
        vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    }
    else
    {
        //mGraphicsPipieline->SetPrimitiveType(ConvertToVulkanPrimitiveTopology(mode));
    }
    //BindPipeline();
    
    vkCmdDrawIndexed(mCommandBuffer, size, 1, offset, baseVertex, 0);
}

void VKRenderEncoder::DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, uint32_t firstInstance, uint32_t instanceCount)
{
	VKIndexBuffer* indexBuffer = (VKIndexBuffer*)buffer.get();
	if (!indexBuffer)
	{
		return;
	}

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
	if (mContext->vulkanExtension.enabledExtendedDynamicState)
	{
		vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
	}
	else
	{
		//mGraphicsPipieline->SetPrimitiveType(ConvertToVulkanPrimitiveTopology(mode));
	}

	vkCmdDrawIndexed(mCommandBuffer, size, instanceCount, offset, 0, firstInstance);
}

// RCBuffer版本的间接绘制
void VKRenderEncoder::DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset, uint32_t drawCount, uint32_t stride)
{
    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (!vkBuffer || !vkBuffer->GetVkBuffer())
    {
        return;
    }

    if (mContext->vulkanExtension.enabledExtendedDynamicState)
    {
        vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    }

    vkCmdDrawIndirect(mCommandBuffer, vkBuffer->GetVkBuffer(), offset, drawCount, stride);
}

void VKRenderEncoder::DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
    int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
    uint32_t drawCount, uint32_t stride)
{
    // Vulkan: index buffer 通过 vkCmdBindIndexBuffer 绑定
    if (indexBuffer)
    {
        VKIndexBufferPtr vkIndexBuffer = std::dynamic_pointer_cast<VKIndexBuffer>(indexBuffer);
        if (vkIndexBuffer && vkIndexBuffer->GetBuffer())
        {
            IndexType type = vkIndexBuffer->getIndexType();
            VkIndexType vkIndexType = (type == IndexType_UInt) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
            int byteOffset = indexBufferOffset * ((type == IndexType_UInt) ? sizeof(uint32_t) : sizeof(uint16_t));
            vkCmdBindIndexBuffer(mCommandBuffer, vkIndexBuffer->GetBuffer(), byteOffset, vkIndexType);
        }
    }

    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(indirectBuffer);
    if (!vkBuffer || !vkBuffer->GetVkBuffer())
    {
        return;
    }

    if (mContext->vulkanExtension.enabledExtendedDynamicState)
    {
        vkCmdSetPrimitiveTopologyEXT(mCommandBuffer, ConvertToVulkanPrimitiveTopology(mode));
    }

    vkCmdDrawIndexedIndirect(mCommandBuffer, vkBuffer->GetVkBuffer(), indirectBufferOffset, drawCount, stride);
}

void VKRenderEncoder::SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler)
{
    VKTextureBasePtr vkTexture = std::dynamic_pointer_cast<VKTextureBase>(texture);
    VKTextureSampler* vkSampler = (VKTextureSampler*)sampler.get();

    VKGraphicsShaderPtr shader = mGraphicsPipieline->GetCurrentShader();
    if (!shader)
    {
        return;
    }

    if (vkTexture)
    {
        // 根据纹理格式选择正确的布局
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (VulkanBufferUtil::IsDepthStencilFormat(vkTexture->GetVKFormat()))
        {
            imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }

        ShaderImageDesc imageDesc;
        imageDesc.image = vkTexture->GetImageView()->GetHandle();
        imageDesc.imageLayout = imageLayout;
        imageDesc.sampler = VK_NULL_HANDLE;

        shader->BindTexture(mCommandBuffer, resourceName, imageDesc, mGraphicsPipieline->GetPipelineLayout());
    }

    if (vkSampler)
    {
        shader->BindSampler(mCommandBuffer, resourceName + "Sam", vkSampler->GetVKSampler(), mGraphicsPipieline->GetPipelineLayout());
    }
}

void VKRenderEncoder::SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler)
{
    // Vulkan uses combined image samplers in the descriptor set; the same binding
    // is accessible from any shader stage. Just delegate to the fragment version.
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

void VKRenderEncoder::SetMeshTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler)
{
    // Vulkan descriptor sets are pipeline-level and shared across all stages.
    // The combined image sampler binding is accessible from mesh stage as well.
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

void VKRenderEncoder::SetTaskTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler)
{
    // Vulkan descriptor sets are pipeline-level and shared across all stages.
    // The combined image sampler binding is accessible from task stage as well.
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

void VKRenderEncoder::DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    if (mContext->vulkanExtension.enableMeshShaderEXT)
    {
        vkCmdDrawMeshTasksEXT(mCommandBuffer, groupCountX, groupCountY, groupCountZ);
    }
}

void VKRenderEncoder::DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                            uint32_t drawCount, uint32_t stride)
{
    if (!buffer)
    {
        return;
    }
    
    VKRCBufferPtr vkBuffer = std::dynamic_pointer_cast<VKRCBuffer>(buffer);
    if (!vkBuffer || !vkBuffer->GetVkBuffer())
    {
        return;
    }
    
    if (mContext->vulkanExtension.enableMeshShaderEXT)
    {
        vkCmdDrawMeshTasksIndirectEXT(mCommandBuffer, vkBuffer->GetVkBuffer(), offset, drawCount, stride);
    }
}

// ===== 动态渲染状态实现 =====

void VKRenderEncoder::SetScissorRect(int x, int y, uint32_t width, uint32_t height)
{
    VkRect2D scissor;
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);
}

void VKRenderEncoder::SetDepthBias(float bias, float slopeScale, float clamp)
{
    vkCmdSetDepthBias(mCommandBuffer, bias, clamp, slopeScale);
}

void VKRenderEncoder::SetStencilReference(uint32_t frontRef, uint32_t backRef)
{
    vkCmdSetStencilReference(mCommandBuffer, VK_STENCIL_FACE_FRONT_BIT, frontRef);
    vkCmdSetStencilReference(mCommandBuffer, VK_STENCIL_FACE_BACK_BIT, backRef);
}

NAMESPACE_RENDERCORE_END
