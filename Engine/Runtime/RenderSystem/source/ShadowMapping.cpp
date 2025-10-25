//
//  ShadowMapping.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/15.
//

#include "ShadowMapping.h"

static const char* vShadowMapShaderStr =
R"(
#version 300 es
uniform mat4 u_mvpLightMatrix;
layout(location = 0) in vec4 a_position;

out vec4 v_color;

void main()
{
    gl_Position = u_mvpLightMatrix * a_position;
}
)";

static const char* fShadowMapShaderStr = R"(
   #version 300 es
   precision lowp float;
   void main()
   {
   }
)";

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

ShadowMapping::ShadowMapping(const RenderDevicePtr& renderDevice)
{
    assert(renderDevice);
    
    //创建深度纹理
    m_DepthTexture = renderDevice->CreateTexture2D(kTexFormatDepth24,
                                            TextureUsage::TextureUsageRenderTarget, 1024, 1024, 1);
    
    //创建采样函数
    SamplerDescriptor samplerDescriptor;
    samplerDescriptor.filterMin = MIN_LINEAR;
    samplerDescriptor.filterMag = MAG_NEAREST;
    m_DepthTextureSampler = renderDevice->CreateSamplerWithDescriptor(samplerDescriptor);
    
    //创建阴影渲染管线
    GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    VertextAttributesDescritptor vertextAttributesDescritptor;
    vertextAttributesDescritptor.index = 0;
    vertextAttributesDescritptor.offset = 0;
    vertextAttributesDescritptor.format = VertexFormatFloat3;
    graphicsPipelineDescriptor.vertexDescriptor.attributes.push_back(vertextAttributesDescritptor);
    
    //graphicsPipelineDescriptor.colorAttachmentDescriptor.writeMask = ColorWriteMaskNone;
    m_GraphicsPipeline = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
    
//    ShaderFunctionPtr vertShader = renderDevice->createShaderFunction(vShadowMapShaderStr, ShaderStage_Vertex);
//    ShaderFunctionPtr fragShader = renderDevice->createShaderFunction(fShadowMapShaderStr, ShaderStage_Fragment);
//    m_GraphicsPipeline->attachVertexShader(vertShader);
//    m_GraphicsPipeline->attachFragmentShader(fragShader);
}

ShadowMapping::~ShadowMapping()
{
    //
}

void ShadowMapping::SetUp()
{
    //
}



NS_RENDERSYSTEM_END
