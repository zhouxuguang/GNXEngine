//
//  FXAAPass.cpp
//  GNXEngine
//

#include "PostProcess/FXAAPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/LogService.h"

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

FXAAPass::FXAAPass()
{
}

FXAAPass::~FXAAPass()
{
}

bool FXAAPass::Initialize()
{
    if (mInitialized)
    {
        return true;
    }

    RenderDevicePtr renderDevice = RenderCore::GetRenderDevice();
    if (!renderDevice)
    {
        return false;
    }

    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("FXAA");
    if (!shaderInfo.graphicsShader)
    {
        LOG_ERROR("FXAAPass: failed to load FXAA shader asset");
        return false;
    }

    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;

    mPipeline = renderDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    if (!mPipeline)
    {
        LOG_ERROR("FXAAPass: failed to create pipeline");
        return false;
    }
    mPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);

    SamplerDesc samplerDesc;
    samplerDesc.filterMin = MIN_LINEAR;
    samplerDesc.filterMag = MAG_LINEAR;
    samplerDesc.wrapS = CLAMP_TO_EDGE;
    samplerDesc.wrapT = CLAMP_TO_EDGE;
    mSampler = renderDevice->CreateSamplerWithDescriptor(samplerDesc);

    mInitialized = true;
    return true;
}

void FXAAPass::Process(const RenderEncoderPtr& renderEncoder, RCTexturePtr inputTexture)
{
    if (!mInitialized || !renderEncoder || !inputTexture)
    {
        return;
    }

    renderEncoder->SetGraphicsPipeline(mPipeline);
    renderEncoder->SetFragmentTextureAndSampler("texImage", inputTexture, mSampler);
    renderEncoder->DrawPrimitives(PrimitiveMode_TRIANGLES, 0, 3);
}

NS_RENDERSYSTEM_END
