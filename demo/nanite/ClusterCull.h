//
//  ClusterCull.h
//  nanite
//
//  Created by zhouxuguang on 2025/11/15.
//

#ifndef NANITE_CLUSTER_CULL_INCLUDE_JDJKSDGFDGDFH
#define NANITE_CLUSTER_CULL_INCLUDE_JDJKSDGFDGDFH

#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/BaseLib/include/BaseLib.h"

//init cluster selection
void InitClusterCullPass(RenderCore::RenderDevicePtr renderDevice);

//cluster cull pass
void ExecuteClusterCullPass(RenderCore::CommandBufferPtr commandBuffer,
	RenderCore::ComputeBufferPtr mainAndPostNodeAndClusterBatches,
	RenderCore::ComputeBufferPtr workArgs,
	RenderCore::ComputeBufferPtr queueState,
	RenderCore::ComputeBufferPtr outVisibleClustersSWHW,
	RenderCore::UniformBufferPtr globalBuffer);

#endif /* NANITE_CLUSTER_CULL_INCLUDE_JDJKSDGFDGDFH */
