#include "HardwareRasterization.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"

static RenderCore::GraphicsPipelinePtr sPSO = nullptr;

void InitHWRasterizePass(RenderCore::RenderDevicePtr renderDevice, RenderCore::ComputeBufferPtr cluterPageData, RenderCore::RCTexture2DPtr visBuffer64)
{
	RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/HWRasterize");

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

void ExecuteHWRasterizePass(RenderCore::CommandBufferPtr commandBuffer, RenderCore::RenderEncoderPtr renderEncoder1, RenderCore::RCTexture2DPtr visBuffer64)
{
	float color[4] = { 0.0, 1.0, 0.0, 1.0 };
	SCOPED_DEBUGMARKER_EVENT(commandBuffer, "HWRasterize", color);

    RenderPass renderPass;
    RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
    colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
    colorAttachmentPtr->texture = visBuffer64;
    renderPass.colorAttachments.push_back(colorAttachmentPtr);

    renderPass.renderRegion = Rect2D(0, 0, 1400, 480);
    RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

	renderEncoder->SetGraphicsPipeline(sPSO);
    
    renderEncoder->EndEncode();
}