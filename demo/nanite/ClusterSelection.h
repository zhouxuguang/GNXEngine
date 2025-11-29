//
//  ClusterSelection.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef ClusterSelection_hpp
#define ClusterSelection_hpp

#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/BaseLib.h"

RenderCore::ComputeBufferPtr InitHierarchyBuffer(RenderCore::RenderDevicePtr renderDevice);

//init cluster selection
void InitClusterSelectionPass(RenderCore::RenderDevicePtr renderDevice);

//cluster selection pass
void ExecuteClusterSelectionPass(RenderCore::CommandBufferPtr commandBuffer,
                                 RenderCore::ComputeBufferPtr hierarchyBuffer,
                                 RenderCore::ComputeBufferPtr outResult,
                                 RenderCore::ComputeBufferPtr rasterBinMeta,
                                 RenderCore::ComputeBufferPtr mainAndPostNodeAndClusterBatches,
                                 RenderCore::UniformBufferPtr globalBuffer);

#endif /* ClusterSelection_hpp */
