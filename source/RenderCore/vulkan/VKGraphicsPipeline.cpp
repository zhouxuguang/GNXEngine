//
//  VKGraphicsPipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKGraphicsPipeline.h"
#include "VKDepthStencilBuffer.h"

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

static VkPipelineColorBlendAttachmentState CreateColorBlendState(const ColorAttachmentDescriptor& colorDes)
{
    VkPipelineColorBlendAttachmentState colorBlendState;
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

VKGraphicsPipeline::VKGraphicsPipeline(VulkanContextPtr context, const GraphicsPipelineDescriptor& des) : GraphicsPipeline(des), mGraphicsPipelineDes(des)
{
    mContext = context;
    memset(&mPipeCreateInfo, 0, sizeof(mPipeCreateInfo));
}

void VKGraphicsPipeline::attachVertexShader(ShaderFunctionPtr shaderFunction)
{
    mShaders.push_back(std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction));
}

void VKGraphicsPipeline::attachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    mShaders.push_back(std::dynamic_pointer_cast<VKShaderFunction>(shaderFunction));
}

void VKGraphicsPipeline::Generate()
{
    if (mGenerated)
    {
        return;
    }
    ContructDes();
    mGenerated = true;
}

void VKGraphicsPipeline::ContructDes()
{
    mPipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // 1、shader信息配置
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (auto &iter : mShaders)
    {
        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = iter->GetVKShaderStage();
        shaderStage.module = iter->GetShaderModule();
        shaderStage.pName = iter->GetEntryName().c_str();
        shaderStages.push_back(shaderStage);
    }
    
    mPipeCreateInfo.stageCount = (uint32_t)shaderStages.size();
    mPipeCreateInfo.pStages = shaderStages.data();

    //2、顶点输入状态
    const VertexInputLayout* vertexInputLayout = nullptr;
    for (auto &iter : mShaders)
    {
        if (iter->GetVKShaderStage() == VK_SHADER_STAGE_VERTEX_BIT)
        {
            vertexInputLayout = &iter->GetVertexInputLayout();
        }
    }
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexInputLayout->inputBindings.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexInputLayout->inputBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInputLayout->attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputLayout->attributeDescriptions.data();
    mPipeCreateInfo.pVertexInputState = &vertexInputInfo;

    //3、顶点输入组装
    VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo = {};
    vertexAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;     //todo 这里还需要根据实际情况修改
    vertexAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    mPipeCreateInfo.pInputAssemblyState = &vertexAssemblyCreateInfo;

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
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;   //注意这里设置为逆时针，后续适配负的高度
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
    colorBlendInfo.attachmentCount = 1;
    VkPipelineColorBlendAttachmentState colorBlendState = CreateColorBlendState(mGraphicsPipelineDes.colorAttachmentDescriptor);
    colorBlendInfo.pAttachments = &colorBlendState;  //
    mPipeCreateInfo.pColorBlendState = &colorBlendInfo;

    //10、动态状态
    std::vector<VkDynamicState> dynamicStates;
    dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    mPipeCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //11、pipelayout
    CreatePipelineLayout();
    mPipeCreateInfo.layout = mPipelineLayout;

    //12、例如dynamic rendering相关的
    
    VkResult result = vkCreateGraphicsPipelines(mContext->device, VK_NULL_HANDLE, 1, &mPipeCreateInfo, nullptr, &mPipeline);
    assert(result == VK_SUCCESS);
}

void VKGraphicsPipeline::CreatePipelineLayout()
{
    // 根据shader反射出来的DescriptorSet信息创建各个DescriptorSetLayout
    DescriptorSetLayoutDataVec desSetLayouts;
    for (auto &iter : mShaders)
    {
        const DescriptorSetLayoutDataVec& desSetLayout = iter->GetDescriptorSets();
        desSetLayouts.insert(desSetLayouts.end(), desSetLayout.begin(), desSetLayout.end());
    }
    
    std::vector<VkDescriptorSetLayout> desLayouts;
    desLayouts.resize(desSetLayouts.size());
    for (int i = 0; i < desSetLayouts.size(); i ++)
    {
        if (vkCreateDescriptorSetLayout(mContext->device, &desSetLayouts[i].create_info, nullptr, desLayouts.data() + i) != VK_SUCCESS)
        {
            printf("failed to create compute descriptor set layout!\n");
        }
    }

    // 创建管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)desLayouts.size();
    pipelineLayoutInfo.pSetLayouts = desLayouts.data();

    if (vkCreatePipelineLayout(mContext->device, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        printf("failed to create compute pipeline layout!");
    }
}

NAMESPACE_RENDERCORE_END
