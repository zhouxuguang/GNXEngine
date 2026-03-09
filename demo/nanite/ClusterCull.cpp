//
//  ClusterCull.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "ClusterCull.h"
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
                                 RenderCore::RCBufferPtr mainAndPostNodeAndClusterBatches,
                                 RenderCore::RCBufferPtr workArgs,
                                 RenderCore::RCBufferPtr clusterPageData,
                                 RenderCore::RCBufferPtr outVisibleClustersSWHW,
                                 RenderCore::UniformBufferPtr globalBuffer)
{
    float color[4] = {1.0, 0.0, 0.0, 1.0};
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, "ClusterCull", color);
    
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(sPSO);
    computeEncoder->SetStorageBuffer(mainAndPostNodeAndClusterBatches, 0);
    computeEncoder->SetStorageBuffer(clusterPageData, 1);
    computeEncoder->SetStorageBuffer(workArgs, 2);
    computeEncoder->SetStorageBuffer(outVisibleClustersSWHW, 3);

    computeEncoder->SetUniformBuffer("GlobalData", globalBuffer);
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    computeEncoder->SetUniformBuffer("cbPerCamera", sceneManager->GetRenderInfo().cameraUBO);

    computeEncoder->Dispatch(1, 1, 1);

    computeEncoder->EndEncode();
}
