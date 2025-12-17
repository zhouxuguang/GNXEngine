//
//  ClusterCull.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NodeAndClusterCull.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

static RenderCore::ComputePipelinePtr sPSO = nullptr;

//init cluster selection
void InitClusterCullPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/ClusterCull");
    sPSO = renderDevice->CreateComputePipeline(*shaderAssetString.computeShader->shaderSource);
}

//cluster selection pass
void ExecuteClusterCullPass(RenderCore::CommandBufferPtr commandBuffer,
                                 RenderCore::ComputeBufferPtr mainAndPostNodeAndClusterBatches,
                                 RenderCore::ComputeBufferPtr workArgs,
                                 RenderCore::ComputeBufferPtr queueState,
                                 RenderCore::ComputeBufferPtr outVisibleClustersSWHW,
                                 RenderCore::UniformBufferPtr globalBuffer)
{
    float color[4] = {1.0, 0.0, 0.0, 1.0};
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, "ClusterCull", color);
    
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(sPSO);
    computeEncoder->SetBuffer(mainAndPostNodeAndClusterBatches, 0);
    computeEncoder->SetBuffer(workArgs, 1);
    //computeEncoder->SetBuffer(queueState, 2);
    computeEncoder->SetBuffer(outVisibleClustersSWHW, 2);
    //computeEncoder->SetUniformBuffer("GlobalData", globalBuffer);

    computeEncoder->Dispatch(1, 1, 1);

    computeEncoder->EndEncode();
}
