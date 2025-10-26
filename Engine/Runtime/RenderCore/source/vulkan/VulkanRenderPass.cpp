//
//  VulkanRenderPass.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/6/4.
//

#include "VulkanRenderPass.h"

NAMESPACE_RENDERCORE_BEGIN

VulkanRenderPass::VulkanRenderPass(VulkanContextPtr context, const RenderPassFormat& passFormat)
{
    mContext = context;
    CreateRenderPass(context, passFormat);
}

VulkanRenderPass::~VulkanRenderPass()
{
    //
}

void VulkanRenderPass::CreateRenderPass(VulkanContextPtr context, const RenderPassFormat& passFormat)
{
    //颜色缓冲描述
    std::vector<VkAttachmentDescription> colorAttachmentDesVec;
    std::vector<VkAttachmentReference> colourAttachmentRefs;
    for (size_t i = 0; i < passFormat.colorFormats.size(); i ++)
    {
        VkAttachmentDescription colorAttachmentDes = {};
        colorAttachmentDes.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
        colorAttachmentDes.format = passFormat.colorFormats[i];
        colorAttachmentDes.samples = context->numSamples;
        colorAttachmentDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        if (context->numSamples > VK_SAMPLE_COUNT_1_BIT)
        {
            colorAttachmentDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        else
        {
            colorAttachmentDes.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        colorAttachmentDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDes.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;       //这里还要根据是否上屏的pass区分不同情况
        
        colorAttachmentDesVec.push_back(colorAttachmentDes);
        
        VkAttachmentReference colourAttachmentRef = {};
        colourAttachmentRef.attachment = (uint32_t)i;
        colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colourAttachmentRefs.push_back(colourAttachmentRef);
    }
    
    //深度缓冲描述
    VkAttachmentDescription depthAttachmentDes = {};
    depthAttachmentDes.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    depthAttachmentDes.format = passFormat.depthFormat;
    depthAttachmentDes.samples = context->numSamples;
    depthAttachmentDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDes.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = (uint32_t)passFormat.colorFormats.size();
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    //模板缓冲描述
    VkAttachmentDescription stencilAttachmentDes = {};
    stencilAttachmentDes.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    stencilAttachmentDes.format = passFormat.depthFormat;
    stencilAttachmentDes.samples = context->numSamples;
    stencilAttachmentDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencilAttachmentDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencilAttachmentDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencilAttachmentDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencilAttachmentDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    stencilAttachmentDes.finalLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

//    VkAttachmentReference colorAttachmentResolveRef = {};
//    colorAttachmentResolveRef.attachment = 2;
//    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = (uint32_t)colourAttachmentRefs.size();
    subpassDescription.pColorAttachments = colourAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

    //子流程依赖，用于变换图像的布局
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = std::move(colorAttachmentDesVec);
    attachments.push_back(depthAttachmentDes);
    attachments.push_back(stencilAttachmentDes);
    
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    vkCreateRenderPass(mContext->device, &renderPassCreateInfo, nullptr, &mRenderPass);
}

NAMESPACE_RENDERCORE_END
