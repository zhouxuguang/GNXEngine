//
//  PostProcessing.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/21.
//

#include "PostProcessing.h"
#include "MathUtil/HalfFloat.h"
#include "../ShaderAssetLoader.h"

NS_RENDERSYSTEM_BEGIN

PostProcessing::PostProcessing(RenderDevicePtr renderDevice)
{
    assert(renderDevice);
    mRenderDevice = renderDevice;
    mTextures[0] = nullptr;
    mTextures[1] = nullptr;

    
    SamplerDescriptor samplerDescriptor;
    mTextureSampler = renderDevice->createSamplerWithDescriptor(samplerDescriptor);
    
    ShaderAssetString shaderAssetString = LoadShaderAsset("PostProcessShader");
    
    ShaderCode vertexShader;
    ShaderCode fragmentShader;
    if (renderDevice->getRenderDeviceType() == RenderDeviceType::GLES)
    {
        vertexShader = std::move(shaderAssetString.gles30Shader.vertexShader);
        fragmentShader = std::move(shaderAssetString.gles30Shader.fragmentShader);
    }
    else if (renderDevice->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        vertexShader = std::move(shaderAssetString.metalShader.vertexShader);
        fragmentShader = std::move(shaderAssetString.metalShader.fragmentShader);
    }
    else if (renderDevice->getRenderDeviceType() == RenderDeviceType::VULKAN)
    {
        vertexShader = std::move(shaderAssetString.metalShader.vertexShader);
        fragmentShader = std::move(shaderAssetString.metalShader.fragmentShader);
    }
    ShaderFunctionPtr vertShader = renderDevice->createShaderFunction(vertexShader, ShaderStage_Vertex);
    ShaderFunctionPtr fragShader = renderDevice->createShaderFunction(fragmentShader, ShaderStage_Fragment);
    GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    
    mPipeline = renderDevice->createGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->attachVertexShader(vertShader);
    mPipeline->attachFragmentShader(fragShader);
}

PostProcessing::~PostProcessing()
{
    //
}

void PostProcessing::Process(const RenderEncoderPtr &renderEncoder, const RenderTexturePtr texture)
{
    renderEncoder->setGraphicsPipeline(mPipeline);
    renderEncoder->setFragmentRenderTextureAndSampler(texture, mTextureSampler, 0);
    
    renderEncoder->drawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
