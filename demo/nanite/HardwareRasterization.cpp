#include "HardwareRasterization.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"
#include "Runtime/RenderSystem/include/SceneManager.h"

static RenderCore::GraphicsPipelinePtr sPSO = nullptr;

void InitHWRasterizePass(RenderCore::RenderDevicePtr renderDevice, 
                         RenderCore::ComputeBufferPtr clusterPageData,
                         RenderCore::RCTexture2DPtr visBuffer64)
{
	RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/HWRasterize");

	ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
	ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

	GraphicsShaderPtr shader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

	GraphicsPipelineDescriptor graphicsPipelineDescriptor;
	graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = CompareFunctionAlways;
	graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = false;
	graphicsPipelineDescriptor.depthStencilDescriptor.stencil.stencilEnable = false;

	sPSO = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
	sPSO->AttachGraphicsShader(shader);
}

void ExecuteHWRasterizePass(RenderCore::CommandBufferPtr commandBuffer, 
                            RenderCore::RCTexture2DPtr visBuffer64,
                            RenderCore::ComputeBufferPtr clusterPageData,
                            uint32_t width, uint32_t height)
{
	float color[4] = { 0.0, 1.0, 0.0, 1.0 };
	SCOPED_DEBUGMARKER_EVENT(commandBuffer, "HWRasterize", color);

    RenderPass renderPass;
    RenderPassColorAttachmentPtr colorAttachmentPtr = std::make_shared<RenderPassColorAttachment>();
    colorAttachmentPtr->clearColor = MakeClearColor(0.0, 0.0, 0.0, 1.0);
    colorAttachmentPtr->texture = visBuffer64;
    renderPass.colorAttachments.push_back(colorAttachmentPtr);

    renderPass.renderRegion = Rect2D(0, 0, width, height);
    RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);

	renderEncoder->SetGraphicsPipeline(sPSO);
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", sceneManager->GetRenderInfo().cameraUBO);
    renderEncoder->SetVertexBuffer(clusterPageData, 0, 4);
    renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 384);
    renderEncoder->EndEncode();
}
