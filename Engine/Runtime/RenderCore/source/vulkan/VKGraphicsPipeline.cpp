//
//  VKGraphicsPipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKGraphicsPipeline.h"
#include "VKDepthStencilBuffer.h"
#include "VulkanDescriptorUtil.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

static VkBlendFactor ConvertToVulkanBlendFactor(BlendFactor blendFactor)
{
    VkBlendFactor vkBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
    switch (blendFactor)
    {
        case BlendFactorZero:
            vkBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
            
        case BlendFactorOne:
            vkBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
            
        case BlendFactorSourceColor:
            vkBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
            break;
            
        case BlendFactorOneMinusSourceColor:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            break;
            
        case BlendFactorSourceAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            break;
            
        case BlendFactorOneMinusSourceAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
            
        case BlendFactorDestinationColor:
            vkBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            break;
            
        case BlendFactorOneMinusDestinationColor:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            break;
            
        case BlendFactorDestinationAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            break;
            
        case BlendFactorOneMinusDestinationAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            break;
            
        case BlendFactorSourceAlphaSaturated:
            vkBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            break;
            
        case BlendFactorBlendColor:
            vkBlendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
            break;
            
        case BlendFactorOneMinusBlendColor:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            break;
            
        case BlendFactorBlendAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
            break;
            
        case BlendFactorOneMinusBlendAlpha:
            vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            break;
            
        default:
            assert(false);
            break;
    }
    
    return vkBlendFactor;
}

static VkBlendOp ConvertToVulkanBlendOp(BlendEquation blendEq)
{
    VkBlendOp vkBlendOp = VK_BLEND_OP_MAX_ENUM;
    switch (blendEq)
    {
        case BlendEquationAdd:
            vkBlendOp = VK_BLEND_OP_ADD;
            break;
            
        case BlendEquationSubtract:
            vkBlendOp = VK_BLEND_OP_SUBTRACT;
            break;
            
        case BlendEquationReverseSubtract:
            vkBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
            break;
            
        case BlendEquationMinimum:
            vkBlendOp = VK_BLEND_OP_MIN;
            break;
            
        case BlendEquationMaximum:
            vkBlendOp = VK_BLEND_OP_MAX;
            break;
            
        default:
            assert(false);
            break;
    }
    
    return vkBlendOp;
}

static VkColorComponentFlags ConvertToVulkanColorWriteMask(ColorWriteMask colorMask)
{
    VkColorComponentFlags writeMask = 0x0;
    if (ColorWriteMaskRed & colorMask)
    {
        writeMask |= VK_COLOR_COMPONENT_R_BIT;
    }
    if (ColorWriteMaskGreen & colorMask)
    {
        writeMask |= VK_COLOR_COMPONENT_G_BIT;
    }
    if (ColorWriteMaskBlue & colorMask)
    {
        writeMask |= VK_COLOR_COMPONENT_B_BIT;
    }
    if (ColorWriteMaskAlpha & colorMask)
    {
        writeMask |= VK_COLOR_COMPONENT_A_BIT;
    }
    return writeMask;
}

static VkPipelineColorBlendAttachmentState CreateColorBlendState(const ColorAttachmentDesc& colorDes)
{
    VkPipelineColorBlendAttachmentState colorBlendState = {};
    colorBlendState.colorWriteMask = ConvertToVulkanColorWriteMask(colorDes.writeMask);
    colorBlendState.blendEnable = colorDes.blendingEnabled;
    colorBlendState.srcColorBlendFactor = ConvertToVulkanBlendFactor(colorDes.sourceRGBBlendFactor);
    colorBlendState.dstColorBlendFactor = ConvertToVulkanBlendFactor(colorDes.destinationRGBBlendFactor);
    colorBlendState.srcAlphaBlendFactor = ConvertToVulkanBlendFactor(colorDes.sourceAlphaBlendFactor);
    colorBlendState.dstAlphaBlendFactor = ConvertToVulkanBlendFactor(colorDes.destinationAplhaBlendFactor);
    colorBlendState.colorBlendOp = ConvertToVulkanBlendOp(colorDes.rgbBlendOperation);
    colorBlendState.alphaBlendOp = ConvertToVulkanBlendOp(colorDes.aplhaBlendOperation);
    
    return colorBlendState;
}

VKGraphicsPipeline::VKGraphicsPipeline(VulkanContextPtr context, const GraphicsPipelineDesc& des) : GraphicsPipeline(des), mGraphicsPipelineDes(des)
{
    mContext = context;
    memset(&mPipeCreateInfo, 0, sizeof(mPipeCreateInfo));
    memset(mStageSetOffsets, 0, ShaderStage_Max * DESCRIPTOR_TYPE_MAX * sizeof(uint32_t));
    
    for (uint32_t i = 0; i < des.renderTargetCount; i ++)
    {
        mColorAttachmentDescs.push_back(des.colorAttachmentDescriptors[i]);
    }
}

void VKGraphicsPipeline::AttachVertexShader(ShaderFunctionPtr shaderFunction)
{
    mShaders.push_back(std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction));
}

void VKGraphicsPipeline::AttachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    mShaders.push_back(std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction));
}

void VKGraphicsPipeline::AttachGraphicsShader(GraphicsShaderPtr graphicsShader)
{
    mShader = std::dynamic_pointer_cast<VKGraphicsShader>(graphicsShader);
}

void VKGraphicsPipeline::AttachTaskShader(ShaderFunctionPtr shaderFunction)
{
    if (shaderFunction)
    {
        mTaskShader = std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction);
        mShaders.push_back(mTaskShader);
    }
}

void VKGraphicsPipeline::AttachMeshShader(ShaderFunctionPtr shaderFunction)
{
    if (shaderFunction)
    {
        mMeshShader = std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction);
        mShaders.push_back(mMeshShader);
    }
}

void VKGraphicsPipeline::Generate(const RenderPassFormat& passFormat)
{
    if (mGenerated)
    {
        return;
    }
    ContructDes(passFormat);
    mGenerated = true;
}

void VKGraphicsPipeline::ContructDes(const RenderPassFormat& passFormat)
{
    mPipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // 1、shader信息配置
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    if (mGraphicsPipelineDes.pipelineType == PipelineType::Mesh)
    {
        // Mesh Pipeline：task + mesh + fragment shader stages
        if (mTaskShader)
        {
            VkPipelineShaderStageCreateInfo taskShaderStage = {};
            taskShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            taskShaderStage.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
            taskShaderStage.module = mTaskShader->GetShaderModule();
            taskShaderStage.pName = mTaskShader->GetEntryName().c_str();
            shaderStages.push_back(taskShaderStage);
        }

        if (mMeshShader)
        {
            VkPipelineShaderStageCreateInfo meshShaderStage = {};
            meshShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            meshShaderStage.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
            meshShaderStage.module = mMeshShader->GetShaderModule();
            meshShaderStage.pName = mMeshShader->GetEntryName().c_str();
            shaderStages.push_back(meshShaderStage);
        }

        // Fragment shader（从 mShaders 中找到 fragment stage 的 shader）
        for (auto& shaderFunc : mShaders)
        {
            if (shaderFunc && shaderFunc->GetShaderStage() == ShaderStage_Fragment)
            {
                VkPipelineShaderStageCreateInfo fragShaderStage = {};
                fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                fragShaderStage.module = shaderFunc->GetShaderModule();
                fragShaderStage.pName = shaderFunc->GetEntryName().c_str();
                shaderStages.push_back(fragShaderStage);
                break;
            }
        }
    }
    else
    {
        // 传统 Graphics Pipeline：vertex + fragment shader stages
        VkPipelineShaderStageCreateInfo vertexShaderStage = {};
        vertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStage.module = mShader->GetVertexShaderModule();
        vertexShaderStage.pName = mShader->GetVertexEntryName().c_str();
        shaderStages.push_back(vertexShaderStage);

        VkPipelineShaderStageCreateInfo fragmentShaderStage = {};
        fragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStage.module = mShader->GetFragmentShaderModule();
        fragmentShaderStage.pName = mShader->GetFragmentEntryName().c_str();
        shaderStages.push_back(fragmentShaderStage);
    }

    mPipeCreateInfo.stageCount = (uint32_t)shaderStages.size();
    mPipeCreateInfo.pStages = shaderStages.data();

    //2、顶点输入状态
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (mGraphicsPipelineDes.pipelineType == PipelineType::Mesh)
    {
        // Mesh pipeline 不使用顶点输入，使用空状态
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
    }
    else
    {
        VertexInputLayout vertexInputLayout = mShader->GetVertexInputLayout();

        // 如果 GraphicsPipelineDesc 中指定了顶点属性格式，则覆盖 shader 反射的格式
        // 这对于 byte4 -> float4 归一化等场景非常重要
        const auto& pipelineAttributes = mGraphicsPipelineDes.vertexDescriptor.attributes;
        if (!pipelineAttributes.empty())
        {
            for (const auto& attr : pipelineAttributes)
            {
                for (auto& vertexAttr : vertexInputLayout.attributeDescriptions)
                {
                    if (vertexAttr.location == attr.index)
                    {
                        vertexAttr.format = VulkanBufferUtil::ConvertVertexFormat(attr.format);
                        // 更新对应的 binding stride
                        uint32_t stride = VulkanBufferUtil::GetVertexFormatSize(attr.format);
                        for (auto& binding : vertexInputLayout.inputBindings)
                        {
                            if (binding.binding == vertexAttr.binding)
                            {
                                binding.stride = stride;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }

        vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexInputLayout.inputBindings.size();
        vertexInputInfo.pVertexBindingDescriptions = vertexInputLayout.inputBindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInputLayout.attributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = vertexInputLayout.attributeDescriptions.data();
    }
    mPipeCreateInfo.pVertexInputState = &vertexInputInfo;

    //3、顶点输入组装
    VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo = {};
    vertexAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;     //todo 这里还需要根据实际情况修改
    vertexAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    if (mGraphicsPipelineDes.pipelineType == PipelineType::Mesh)
    {
        // Mesh Pipeline 不使用输入装配状态，设置为 nullptr
        mPipeCreateInfo.pInputAssemblyState = nullptr;
        mPipeCreateInfo.pVertexInputState = nullptr;
    }
    else
    {
        mPipeCreateInfo.pInputAssemblyState = &vertexAssemblyCreateInfo;
    }

    //4、细分状态
    mPipeCreateInfo.pTessellationState = NULL;

    //5、视口状态
    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.viewportCount = 1;

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = 1;
    viewport.height = 1;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D rect2D;
    rect2D.offset.x = 0;
    rect2D.offset.y = 0;
    rect2D.extent.height = 1;
    rect2D.extent.width = 1;

    pipelineViewportStateCreateInfo.pViewports = &viewport;
    pipelineViewportStateCreateInfo.pScissors = &rect2D;

    mPipeCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;

    //6、光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterInfo = {};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = (mGraphicsPipelineDes.fillMode == FillModeWireframe) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_NONE;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;   //注意这里设置为逆时针
    rasterInfo.depthBiasEnable = VK_TRUE;
    rasterInfo.lineWidth = 1;
    mPipeCreateInfo.pRasterizationState = &rasterInfo;

    //7、多重采样信息
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multiSampleInfo = {};
    multiSampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multiSampleInfo.sampleShadingEnable = VK_FALSE;
    multiSampleInfo.minSampleShading = 0;
    multiSampleInfo.pSampleMask = &sampleMask;
    multiSampleInfo.alphaToCoverageEnable = VK_FALSE;
    multiSampleInfo.alphaToOneEnable = VK_FALSE;
    mPipeCreateInfo.pMultisampleState = &multiSampleInfo;

    //8、深度和模板状态
    VKDepthStencilState depthStencilDes = VKDepthStencilState(mGraphicsPipelineDes.depthStencilDescriptor);
    mPipeCreateInfo.pDepthStencilState = &depthStencilDes.GetDepthStencilStateCreateInfo();

    //9、颜色混合状态
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = (uint32_t)mColorAttachmentDescs.size();
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendStates;
    for (size_t i = 0; i < mColorAttachmentDescs.size(); i ++)
    {
        colorBlendStates.push_back(CreateColorBlendState(mColorAttachmentDescs[i]));
    }
    colorBlendInfo.pAttachments = colorBlendStates.data();
    mPipeCreateInfo.pColorBlendState = &colorBlendInfo;

    //10、动态状态
    std::vector<VkDynamicState> dynamicStates;
    dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    //dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);

    // Mesh Pipeline 不使用输入装配，因此不需要 PRIMITIVE_TOPOLOGY 动态状态
    if (mContext->vulkanExtension.enabledExtendedDynamicState &&
        mGraphicsPipelineDes.pipelineType != PipelineType::Mesh)
    {
        dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
    }

    if (mContext->vulkanExtension.enabledExtendedDynamicState3)
    {
        dynamicStates.push_back(VK_DYNAMIC_STATE_POLYGON_MODE_EXT);
    }
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    mPipeCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //11、pipelayout
    CreatePipelineLayout();
    mPipeCreateInfo.layout = mPipelineLayout;

    //12、例如dynamic rendering相关的
    std::vector<VkPipelineRenderingCreateInfoKHR> renderingCreateInfos;
    if (mContext->vulkanExtension.enabledDynamicRendering)
    {
        // New create info to define color, depth and stencil attachments at pipeline create time
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingCreateInfo.colorAttachmentCount = (uint32_t)passFormat.colorFormats.size();
        pipelineRenderingCreateInfo.pColorAttachmentFormats = passFormat.colorFormats.data();
        pipelineRenderingCreateInfo.depthAttachmentFormat = passFormat.depthFormat;
        pipelineRenderingCreateInfo.stencilAttachmentFormat = passFormat.stencilFormat;
        renderingCreateInfos.push_back(pipelineRenderingCreateInfo);
        
        // 动态渲染需要把renderpass设置为空
        mPipeCreateInfo.renderPass = nullptr;
    }
    mPipeCreateInfo.pNext = renderingCreateInfos.data();
    
    VkResult result = vkCreateGraphicsPipelines(mContext->device, mContext->pipelineCache, 1, &mPipeCreateInfo, nullptr, &mPipeline);
    assert(result == VK_SUCCESS);
}

void VKGraphicsPipeline::CreatePipelineLayout()
{
    if (mGraphicsPipelineDes.pipelineType == PipelineType::Mesh)
    {
        // Mesh pipeline：从各个 VKShaderFunction 的反射数据构建管线布局
        // 收集所有 shader stage 的 descriptor set layout data，按 set number 合并
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setGroups;

        auto collectDescriptorSets = [&](const VKShaderFunctionPtr& shaderFunc)
        {
            if (!shaderFunc) return;
            const auto& desSets = shaderFunc->GetDescriptorSets();
            for (const auto& desSet : desSets)
            {
                for (const auto& binding : desSet.bindings)
                {
                    // 合并同一 set 的 binding，用 VK_SHADER_STAGE_ALL 标记
                    auto& groupBindings = setGroups[desSet.set_number];
                    bool found = false;
                    for (auto& existing : groupBindings)
                    {
                        if (existing.binding == binding.binding)
                        {
                            // 同一 binding 已存在，合并 stage flags
                            existing.stageFlags |= binding.stageFlags;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        VkDescriptorSetLayoutBinding newBinding = binding;
                        newBinding.stageFlags = VK_SHADER_STAGE_ALL;
                        groupBindings.push_back(newBinding);
                    }
                }
            }
        };

        collectDescriptorSets(mTaskShader);
        collectDescriptorSets(mMeshShader);
        // Fragment shader（从 mShaders 中找到）
        for (auto& shaderFunc : mShaders)
        {
            if (shaderFunc && shaderFunc->GetShaderStage() == ShaderStage_Fragment)
            {
                collectDescriptorSets(shaderFunc);
                break;
            }
        }

        // 如果没有任何 descriptor，创建一个空的 pipeline layout
        if (setGroups.empty())
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &mPipelineLayout);
            return;
        }

        // 创建 VkDescriptorSetLayout
        bool enablePushDesDescriptor = mContext->vulkanExtension.enablePushDesDescriptor;
        for (const auto& [setIndex, bindings] : setGroups)
        {
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
            layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            if (enablePushDesDescriptor)
            {
                layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
            }
            layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
            layoutCreateInfo.pBindings = bindings.data();
            VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
            vkCreateDescriptorSetLayout(mContext->device, &layoutCreateInfo, nullptr, &setLayout);
            mMeshDescriptorSetLayouts.push_back(setLayout);
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = (uint32_t)mMeshDescriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = mMeshDescriptorSetLayouts.data();

        if (vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
        {
            printf("failed to create mesh pipeline layout!");
        }
    }
    else
    {
        // 传统 Graphics Pipeline：使用 VKGraphicsShader 的 descriptor set layouts
        const std::vector<VkDescriptorSetLayout>& desLayouts = mShader->GetDescriptorSetLayouts();
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = (uint32_t)desLayouts.size();
        pipelineLayoutInfo.pSetLayouts = desLayouts.data();

        if (vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
        {
            printf("failed to create compute pipeline layout!");
        }
    }
}

VKGraphicsShaderPtr VKGraphicsPipeline::GetCurrentShader() const
{
    return mShader;
}

uint32_t VKGraphicsPipeline::GetResourceBindIndex(const std::string& resourceName) const
{
    if (!mShader)
    {
        return (uint32_t)-1;
    }
    return mShader->GetResourceBindIndex(resourceName);
}

uint32_t VKGraphicsPipeline::GetSetOffset(DescriptorType descriptorType) const
{
    if (!mShader)
    {
        return 0;
    }
    return mShader->GetSetOffset(descriptorType);
}

NAMESPACE_RENDERCORE_END
