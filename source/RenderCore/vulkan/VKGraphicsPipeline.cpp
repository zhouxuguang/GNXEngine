//
//  VKGraphicsPipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKGraphicsPipeline.h"

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

VKGraphicsPipeline::VKGraphicsPipeline(VulkanContextPtr context, const GraphicsPipelineDescriptor& des) : GraphicsPipeline(des)
{
    mContext = context;
    memset(&mPipeCreateInfo, 0, sizeof(mPipeCreateInfo));
}

void VKGraphicsPipeline::attachVertexShader(ShaderFunctionPtr shaderFunction)
{
    //
}

void VKGraphicsPipeline::attachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    //
}

void VKGraphicsPipeline::Generate()
{
    //
}

void VKGraphicsPipeline::ContructDes()
{
    mPipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // 1、shader信息配置
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo shaderStages[2]
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        }
    };
    mPipeCreateInfo.stageCount = 2;
    mPipeCreateInfo.pStages = shaderStages;

    //2、顶点输入状态
    std::vector<VkVertexInputBindingDescription> bindingDes;
    std::vector<VkVertexInputAttributeDescription> attributeDes;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDes.size();
    vertexInputInfo.pVertexBindingDescriptions = bindingDes.data();
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDes.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDes.data();
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
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    //multisampleInfo.rasterizationSamples = 1;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.minSampleShading = 0;
    multisampleInfo.pSampleMask = &sampleMask;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;
    mPipeCreateInfo.pMultisampleState = &multisampleInfo;

    //8、深度和模板状态
    mPipeCreateInfo.pDepthStencilState = nullptr;

    //9、颜色混合状态
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = nullptr;  //
    mPipeCreateInfo.pColorBlendState = &colorBlendInfo;

    //10、动态状态
    const VkDynamicState pDynamicStates[] = 
    {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_DEPTH_BIAS,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = sizeof(pDynamicStates)/ sizeof(VkDynamicState);
    dynamicStateCreateInfo.pDynamicStates = pDynamicStates;
    mPipeCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //11、pipelayout
    mPipeCreateInfo.layout = nullptr;

    //12、例如dynamic rendering相关的
}

NAMESPACE_RENDERCORE_END
