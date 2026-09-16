//
//  PostProcessing.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/21.
//

#include "PostProcessing.h"
#include "Runtime/MathUtil/include/HalfFloat.h"
#include "../ShaderAssetLoader.h"

NS_RENDERSYSTEM_BEGIN

PostProcessing::PostProcessing(RenderDevicePtr renderDevice)
{
    assert(renderDevice);
    mRenderDevice = renderDevice;
    
    SamplerDesc samplerDescriptor;
    mTextureSampler = renderDevice->CreateSamplerWithDescriptor(samplerDescriptor);
    
    ShaderAssetString shaderAssetString = LoadShaderAsset("PostProcessShader");
    
    GraphicsShaderPtr graphicsShader = renderDevice->CreateGraphicsShader(
        *shaderAssetString.vertexShader, *shaderAssetString.fragmentShader);

    GraphicsPipelineDesc graphicsPipelineDescriptor;
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
    
    renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
