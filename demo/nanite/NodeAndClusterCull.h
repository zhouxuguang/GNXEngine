//
//  NodeAndClusterCull.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef NANITE_NODEAND_CLUSTER_CULL_INCLUDE_JDJKMDSK
#define NANITE_NODEAND_CLUSTER_CULL_INCLUDE_JDJKMDSK

#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/BaseLib.h"

RenderCore::ComputeBufferPtr InitHierarchyBuffer(RenderCore::RenderDevicePtr renderDevice);

//init cluster selection
void InitNodeAndClusterCullPass(RenderCore::RenderDevicePtr renderDevice);

//cluster selection pass
void ExecuteNodeAndClusterCullPass(RenderCore::CommandBufferPtr commandBuffer,
                                   uint32_t level,
                                 RenderCore::ComputeBufferPtr hierarchyBuffer,
                                 RenderCore::ComputeBufferPtr inWorkArgs,
                                 RenderCore::ComputeBufferPtr outResult,
                                 RenderCore::ComputeBufferPtr mainAndPostNodeAndClusterBatches,
                                 RenderCore::UniformBufferPtr globalBuffer);

#endif /* NANITE_NODEAND_CLUSTER_CULL_INCLUDE_JDJKMDSK */
