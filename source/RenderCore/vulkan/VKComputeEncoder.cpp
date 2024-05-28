//
//  VKComputeEncoder.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputeEncoder.h"
#include "VKComputePipeline.h"
#include "VKComputeBuffer.h"
#include "VKTexture2D.h"
#include "VKRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

VKComputeEncoder::VKComputeEncoder(VulkanContextPtr context, VkCommandBuffer commandBuffer)
{
    mContext = context;
    mCommandBuffer = commandBuffer;
}

VKComputeEncoder::~VKComputeEncoder()
{
}

void VKComputeEncoder::SetComputePipeline(ComputePipelinePtr computePipeline)
{
    if (!computePipeline)
    {
        return;
    }
    VKComputePipeline *vkPipeline = (VKComputePipeline*)computePipeline.get();
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetPipeline());
}

void VKComputeEncoder::SetBuffer(ComputeBufferPtr buffer, uint32_t index)
{
    //
}

void VKComputeEncoder::SetTexture(Texture2DPtr texture, uint32_t index)
{
    //
}

void VKComputeEncoder::SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    //
}

void VKComputeEncoder::Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
{
    vkCmdDispatch(mCommandBuffer, threadGroupsX, threadGroupsY, threadGroupsZ);
}

void VKComputeEncoder::EndEncode()
{
    vkCmdEndRenderingKHR(mCommandBuffer);
}

NAMESPACE_RENDERCORE_END
