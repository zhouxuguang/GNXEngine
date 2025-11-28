#include "Visualization.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

static RenderCore::ComputePipelinePtr sPSO = nullptr;

void InitVisualizationPass(RenderCore::RenderDevicePtr renderDevice)
{
	RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/Visualization");
	sPSO = renderDevice->CreateComputePipeline(*shaderAssetString.computeShader->shaderSource);
}

void ExecuteVisualizationPass(RenderCore::CommandBufferPtr commandBuffer, RenderCore::RCTexture2DPtr visBuffer64, RenderCore::RCTexture2DPtr visualizationBuffer)
{
	float color[4] = { 0.0, 0.0, 1.0, 1.0 };
	SCOPED_DEBUGMARKER_EVENT(commandBuffer, "Visualization", color);

	RenderCore::ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
	computeEncoder->SetComputePipeline(sPSO);
	computeEncoder->SetTexture(visBuffer64, 0);  //global constant buffer
    computeEncoder->SetOutTexture(visualizationBuffer, 1);
    
    uint32_t x, y ,z;
    sPSO->GetThreadGroupSizes(x, y, z);
    
    uint32_t groupX = (visBuffer64->GetWidth() + x - 1) / x;
    uint32_t groupY = (visBuffer64->GetHeight() + y - 1) / y;
    
	computeEncoder->Dispatch(groupX, groupY, 1);

	computeEncoder->EndEncode();
}
