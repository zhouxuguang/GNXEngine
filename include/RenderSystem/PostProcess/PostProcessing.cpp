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
    
    SamplerDescriptor samplerDescriptor;
    mTextureSampler = renderDevice->CreateSamplerWithDescriptor(samplerDescriptor);
    
    ShaderAssetString shaderAssetString = LoadShaderAsset("PostProcessShader");
    
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

    GraphicsShaderPtr graphicsShader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

    GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    
    mPipeline = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->AttachGraphicsShader(graphicsShader);
}

PostProcessing::~PostProcessing()
{
    //
}

void PostProcessing::SetRenderTexture(const RCTexturePtr texture)
{
    mTexture = texture;
}

void PostProcessing::Process(const RenderEncoderPtr &renderEncoder)
{
    renderEncoder->SetGraphicsPipeline(mPipeline);
    renderEncoder->SetFragmentTextureAndSampler("texImage", mTexture, mTextureSampler);
    
    renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
