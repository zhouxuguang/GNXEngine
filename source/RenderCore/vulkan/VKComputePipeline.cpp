//
//  VKComputePipeline.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputePipeline.h"

NAMESPACE_RENDERCORE_BEGIN

VKComputePipeline::VKComputePipeline(VulkanContextPtr context, const char* pszShaderString) : ComputePipeline(nullptr)
{
    //
}

VKComputePipeline::~VKComputePipeline()
{
    //
}

void VKComputePipeline::GetThreadGroupSizes(uint32_t &x, uint32_t &y, uint32_t &z)
{
    return;
}

NAMESPACE_RENDERCORE_END
