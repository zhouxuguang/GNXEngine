#include "RasterClear.h"

#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

static RenderCore::ComputePipelinePtr sPSO = nullptr;

void InitRasterClearPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/RasterClear");
    sPSO = renderDevice->CreateComputePipeline(*shaderAssetString.computeShader->shaderSource);
}

void ExecuteRasterClearPass(RenderCore::CommandBufferPtr commandBuffer,
                            RenderCore::ComputeBufferPtr queueState,
                            RenderCore::ComputeBufferPtr workArgs0,
                            RenderCore::ComputeBufferPtr workArgs1,
                            RenderCore::RCTexture2DPtr visBuffer64)
{
    float color[4] = { 0.5, 0.8, 0.5, 1.0 };
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, "RasterClear", color);

    RenderCore::ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(sPSO);
    computeEncoder->SetOutTexture(visBuffer64, 0);
    computeEncoder->SetBuffer(queueState, 1);
    computeEncoder->SetBuffer(workArgs0, 2);
    computeEncoder->SetBuffer(workArgs1, 3);
    
    uint32_t x, y ,z;
    sPSO->GetThreadGroupSizes(x, y, z);
    
    uint32_t groupX = (visBuffer64->GetWidth() + x - 1) / x;
    uint32_t groupY = (visBuffer64->GetHeight() + y - 1) / y;
    
    computeEncoder->Dispatch(groupX, groupY, 1);

    computeEncoder->EndEncode();
}
