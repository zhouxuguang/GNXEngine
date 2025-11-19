#include "SwapChain.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

static RenderCore::GraphicsPipelinePtr sPSO = nullptr;

void InitSwapChainPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/SwapChainPresent");

	ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
	ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

	GraphicsShaderPtr shader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

	GraphicsPipelineDescriptor graphicsPipelineDescriptor;
	graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
	graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = CompareFunctionLessThanOrEqual;
	graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;

	sPSO = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
	sPSO->AttachGraphicsShader(shader);
}

void ExecuteSwapChainPass(RenderCore::CommandBufferPtr commandBuffer, RenderCore::RenderEncoderPtr renderEncoder)
{
	float color[4] = {0.0, 1.0, 0.0, 1.0};
	SCOPED_DEBUGMARKER_EVENT(commandBuffer, "SwapChain", color);
	renderEncoder->SetGraphicsPipeline(sPSO);
	renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 3);
}
