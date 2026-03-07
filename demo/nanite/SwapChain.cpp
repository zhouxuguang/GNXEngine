#include "SwapChain.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

static RenderCore::GraphicsPipelinePtr sPSO = nullptr;
static RenderCore::TextureSamplerPtr sSam = nullptr;

void InitSwapChainPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/SwapChainPresent");

	ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
	ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

	GraphicsShaderPtr shader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

    GraphicsPipelineDesc graphicsPipelineDescriptor;
	graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
	graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = CompareFunctionGreaterThanOrEqual;
	graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;

	sPSO = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
	sPSO->AttachGraphicsShader(shader);
    
    RenderCore::SamplerDesc samplerDesc;
    sSam = renderDevice->CreateSamplerWithDescriptor(samplerDesc);
}

void ExecuteSwapChainPass(RenderCore::CommandBufferPtr commandBuffer, 
                          RenderCore::RenderEncoderPtr renderEncoder,
                          RCTexture2DPtr visBuffer)
{
	float color[4] = {0.0, 1.0, 0.0, 1.0};
	SCOPED_DEBUGMARKER_EVENT(commandBuffer, "SwapChain", color);
	renderEncoder->SetGraphicsPipeline(sPSO);
    renderEncoder->SetFragmentTextureAndSampler("texImage", visBuffer, sSam);
	renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
}
