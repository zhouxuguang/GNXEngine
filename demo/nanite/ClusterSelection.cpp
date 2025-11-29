//
//  ClusterSelection.cpp
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#include "ClusterSelection.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"

static RenderCore::ComputePipelinePtr sPSO = nullptr;

RenderCore::ComputeBufferPtr InitHierarchyBuffer(RenderCore::RenderDevicePtr renderDevice)
{
    // 加载hierarchyBuffer的文件
    std::string strDataFile = GetProjectAssetDir() + "Nanite/mitsuba.bvh";
    std::vector<uint8_t> hBufferData = baselib::FileUtil::ReadBinaryFile(strDataFile);
    RenderCore::ComputeBufferPtr hierarchyBuffer = renderDevice->CreateComputeBuffer(hBufferData.data(), (uint32_t)hBufferData.size(),
                                                                        RenderCore::StorageMode::StorageModePrivate);
    hierarchyBuffer->SetName("Nanite.HierarchyBuffer");
    return hierarchyBuffer;
}

//init cluster selection
void InitClusterSelectionPass(RenderCore::RenderDevicePtr renderDevice)
{
    RenderSystem::ShaderAssetString shaderAssetString = RenderSystem::LoadShaderAsset("Nanite/ClusterSelection");
    sPSO = renderDevice->CreateComputePipeline(*shaderAssetString.computeShader->shaderSource);
}

//cluster selection pass
void ExecuteClusterSelectionPass(RenderCore::CommandBufferPtr commandBuffer,
                                 RenderCore::ComputeBufferPtr hierarchyBuffer,
                                 RenderCore::ComputeBufferPtr outResult,
                                 RenderCore::ComputeBufferPtr rasterBinMeta,
                                 RenderCore::ComputeBufferPtr mainAndPostNodeAndClusterBatches)
{
    float color[4] = {1.0, 0.0, 0.0, 1.0};
    SCOPED_DEBUGMARKER_EVENT(commandBuffer, "Cluster Selection", color);
    
    ComputeEncoderPtr computeEncoder = commandBuffer->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(sPSO);
    computeEncoder->SetBuffer(hierarchyBuffer, 0);
    computeEncoder->SetBuffer(outResult, 1);
    computeEncoder->SetBuffer(rasterBinMeta, 2);
    computeEncoder->SetBuffer(mainAndPostNodeAndClusterBatches, 3);

    computeEncoder->Dispatch(1, 1, 1);

    computeEncoder->EndEncode();
}
