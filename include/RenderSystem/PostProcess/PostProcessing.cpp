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
    
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
    /*ShaderFunctionPtr vertShader = renderDevice->createShaderFunction(*vertexShader, ShaderStage_Vertex);
    ShaderFunctionPtr fragShader = renderDevice->createShaderFunction(*fragmentShader, ShaderStage_Fragment);*/

    GraphicsShaderPtr graphicsShader = renderDevice->createGraphicsShader(*vertexShader, *fragmentShader);

    GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    
    mPipeline = renderDevice->createGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->attachGraphicsShader(graphicsShader);
}

PostProcessing::~PostProcessing()
{
    //
}

void PostProcessing::SetRenderTexture(const RenderTexturePtr texture)
{
    mTexture = texture;
}

void PostProcessing::Process(const RenderEncoderPtr &renderEncoder)
{
    renderEncoder->setGraphicsPipeline(mPipeline);
    renderEncoder->setFragmentRenderTextureAndSampler("texImage", mTexture, mTextureSampler);
    
    renderEncoder->drawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
