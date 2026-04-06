//
//  ClusterSelection.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "NodeAndClusterCull.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

static RenderCore::ComputePipelinePtr sPSO = nullptr;

RenderCore::RCBufferPtr InitHierarchyBuffer(RenderCore::RenderDevicePtr renderDevice)
{
    // 加载hierarchyBuffer的文件
    std::string strDataFile = GetProjectAssetDir() + "Nanite/mitsuba.bvh";
    std::vector<uint8_t> hBufferData = baselib::FileUtil::ReadBinaryFile(strDataFile);
    RenderCore::RCBufferDesc desc(static_cast<uint32_t>(hBufferData.size()),
        RenderCore::RCBufferUsage::StorageBuffer, RenderCore::StorageMode::StorageModePrivate);
    RenderCore::RCBufferPtr hierarchyBuffer = renderDevice->CreateBuffer(desc, hBufferData.data());
    hierarchyBuffer->SetName("Nanite.HierarchyBuffer");
    return hierarchyBuffer;
}

//init cluster selection
void InitNodeAndClusterCullPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/NodeAndClusterCull");
    sPSO = renderDevice->CreateComputePipeline(*shaderAssetString.computeShader->shaderSource);
}

//cluster selection pass
void ExecuteNodeAndClusterCullPass(RenderCore::CommandBufferPtr commandBuffer,
                                   uint32_t level,
                                 RenderCore::RCBufferPtr hierarchyBuffer,
                                 RenderCore::RCBufferPtr inWorkArgs,
                                 RenderCore::RCBufferPtr outResult,
                                 RenderCore::RCBufferPtr queueState,
                                 RenderCore::RCBufferPtr mainAndPostNodeAndClusterBatches,
                                 RenderCore::UniformBufferPtr globalBuffer)
{
    float color[4] = {1.0, 0.0, 0.0, 1.0};
    
    char buffer[256] = {0};
    snprintf(buffer, 256, "NodeAndClusterCull_%d", level);
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, buffer, color);
    
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(sPSO);
    computeEncoder->SetStorageBuffer("HierarchyBuffer", hierarchyBuffer);
    computeEncoder->SetStorageBuffer("CurrentIndirectArgs", inWorkArgs);
    computeEncoder->SetStorageBuffer("OutResult", outResult);
    computeEncoder->SetStorageBuffer("QueueState", queueState);
    computeEncoder->SetStorageBuffer("OutMainAndPostNodeAndClusterBatches", mainAndPostNodeAndClusterBatches);
    computeEncoder->SetUniformBuffer("GlobalData", globalBuffer);
    
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    computeEncoder->SetUniformBuffer("cbPerCamera", sceneManager->GetRenderInfo().cameraUBO);

    computeEncoder->Dispatch(1, 1, 1);

    computeEncoder->EndEncode();
}
