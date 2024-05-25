//
//  TestPost.cpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/21.
//

#include "TestPost.hpp"
#include "RenderSystem/PostProcess/PostProcessing.h"

static PostProcessing* postProcessing = nullptr;

void initPostResource(rendercore::RenderDevicePtr renderDevice)
{
    postProcessing = new PostProcessing(renderDevice);
}

void testPost(const rendercore::RenderEncoderPtr &renderEncoder, const rendercore::RenderTexturePtr texture)
{
    postProcessing->Process(renderEncoder, texture);
}
