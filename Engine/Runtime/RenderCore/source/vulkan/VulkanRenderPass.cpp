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
        // 颜色附件 loadOp 必须从 passFormat 读取（不能硬编码 CLEAR）：
        // Skybox 等中间 RT 复用 pass 用 LOAD 保留 DeferredLighting 结果，
        // 若硬编码 CLEAR 会把已有颜色清成黑 → 线框/物体变黑。
        colorAttachmentDes.loadOp = (i < passFormat.colorLoadOps.size())
            ? passFormat.colorLoadOps[i] : VK_ATTACHMENT_LOAD_OP_CLEAR;
        if (context->numSamples > VK_SAMPLE_COUNT_1_BIT)
        {
            colorAttachmentDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        else
        {
            colorAttachmentDes.storeOp = (i < passFormat.colorStoreOps.size())
                ? passFormat.colorStoreOps[i] : VK_ATTACHMENT_STORE_OP_STORE;
        }
        colorAttachmentDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // initialLayout：LOAD（保留已有内容）时必须用当前实际布局（而非 UNDEFINED，否则内容未定义）；
        // CLEAR（将被清空）才用 UNDEFINED。
        colorAttachmentDes.initialLayout = (colorAttachmentDes.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        // 上屏 pass 的最终布局是 PRESENT_SRC_KHR，非上屏的中间 RT 则是 SHADER_READ_ONLY_OPTIMAL
        // （渲染后供后续 pass 采样）。这里必须区分，否则中间 RT 的 finalLayout 错误会导致后续
        // 采样时布局不匹配 → 黑屏。
        colorAttachmentDes.finalLayout = passFormat.isPresentStage
            ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        colorAttachmentDesVec.push_back(colorAttachmentDes);
        
        VkAttachmentReference colourAttachmentRef = {};
        colourAttachmentRef.attachment = (uint32_t)i;
        colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colourAttachmentRefs.push_back(colourAttachmentRef);
    }
    
    // 深度/模板附件：只有格式有效（!= VK_FORMAT_UNDEFINED）时才创建。
    // 注意：BeginRenderPass 里的 framebuffer 也只添加实际存在的 image view，
    // 若这里无条件创建深度/模板附件，而 framebuffer 没有对应 attachment，
    // 会导致 vkCreateFramebuffer 附件数不匹配 → 驱动崩溃（SIGSEGV）。
    VkAttachmentDescription depthAttachmentDes = {};
    VkAttachmentDescription stencilAttachmentDes = {};
    bool hasDepth = (passFormat.depthFormat != VK_FORMAT_UNDEFINED);
    bool hasStencil = (passFormat.stencilFormat != VK_FORMAT_UNDEFINED);

    std::vector<VkAttachmentDescription> attachments = std::move(colorAttachmentDesVec);

    // 深度/模板附件的 initialLayout：
    //  - 读写深度（loadOp=CLEAR）：UNDEFINED（内容将被清除）
    //  - 只读深度复用（loadOp=LOAD）：必须用 READ_ONLY 布局（保留已有深度用于深度测试）
    VkImageLayout depthInitialLayout = (passFormat.depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
        ? passFormat.depthLayout : VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout stencilInitialLayout = (passFormat.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
        ? passFormat.stencilLayout : VK_IMAGE_LAYOUT_UNDEFINED;

    if (hasDepth)
    {
        depthAttachmentDes.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
        depthAttachmentDes.format = passFormat.depthFormat;
        depthAttachmentDes.samples = context->numSamples;
        // 只读深度（READ_ONLY 布局）：loadOp 必须为 LOAD，不能 CLEAR
        // （否则 VUID-VkRenderPassCreateInfo-pAttachments-02511）
        bool depthReadOnly = (passFormat.depthLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        depthAttachmentDes.loadOp = depthReadOnly ? VK_ATTACHMENT_LOAD_OP_LOAD : passFormat.depthLoadOp;
        depthAttachmentDes.storeOp = depthReadOnly ? VK_ATTACHMENT_STORE_OP_STORE : passFormat.depthStoreOp;
        depthAttachmentDes.stencilLoadOp = depthReadOnly ? VK_ATTACHMENT_LOAD_OP_LOAD : passFormat.stencilLoadOp;
        depthAttachmentDes.stencilStoreOp = depthReadOnly ? VK_ATTACHMENT_STORE_OP_STORE : passFormat.stencilStoreOp;
        depthAttachmentDes.initialLayout = depthInitialLayout;
        // finalLayout：只读深度保持 READ_ONLY（复用 PreDepth 供后续采样），
        // 读写深度转回 ATTACHMENT_OPTIMAL（后续 pass 再 barrier 转采样布局）
        depthAttachmentDes.finalLayout = passFormat.depthLayout;
        attachments.push_back(depthAttachmentDes);
    }

    if (hasStencil)
    {
        stencilAttachmentDes.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
        stencilAttachmentDes.format = passFormat.stencilFormat;
        stencilAttachmentDes.samples = context->numSamples;
        // 只读模板（READ_ONLY 布局）：loadOp 必须为 LOAD，不能 CLEAR
        bool stencilReadOnly = (passFormat.stencilLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        stencilAttachmentDes.loadOp = stencilReadOnly ? VK_ATTACHMENT_LOAD_OP_LOAD : passFormat.stencilLoadOp;
        stencilAttachmentDes.storeOp = stencilReadOnly ? VK_ATTACHMENT_STORE_OP_STORE : passFormat.stencilStoreOp;
        stencilAttachmentDes.stencilLoadOp = stencilReadOnly ? VK_ATTACHMENT_LOAD_OP_LOAD : passFormat.stencilLoadOp;
        stencilAttachmentDes.stencilStoreOp = stencilReadOnly ? VK_ATTACHMENT_STORE_OP_STORE : passFormat.stencilStoreOp;
        stencilAttachmentDes.initialLayout = stencilInitialLayout;
        // 深度模板通常共用一个附件（格式 D*_S8），finalLayout 必须用 DEPTH_STENCIL 系列布局，
        // 不能用 STENCIL_ATTACHMENT_OPTIMAL（对 DS 组合格式非法 → VUID-VkAttachmentDescription-format-06907）
        stencilAttachmentDes.finalLayout = passFormat.stencilLayout;
        attachments.push_back(stencilAttachmentDes);
    }

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = (uint32_t)colourAttachmentRefs.size();
    subpassDescription.pColorAttachments = colourAttachmentRefs.data();

    // 深度/模板引用：与 attachments 数量对应。只有一个深度/模板附件时，
    // 它同时作为 depth 和 stencil 引用（Vulkan 深度模板通常共用一个附件）。
    // layout 必须与 passFormat.depthLayout/stencilLayout 一致（只读深度用 READ_ONLY）。
    VkAttachmentReference depthStencilAttachmentRef = {};
    depthStencilAttachmentRef.attachment = (uint32_t)passFormat.colorFormats.size();
    depthStencilAttachmentRef.layout = (hasDepth && passFormat.depthLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        ? passFormat.depthLayout
        : ((hasStencil && passFormat.stencilLayout != VK_IMAGE_LAYOUT_UNDEFINED) ? passFormat.stencilLayout : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    if (hasDepth || hasStencil)
    {
        subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentRef;
    }

    //子流程依赖，用于变换图像的布局
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
