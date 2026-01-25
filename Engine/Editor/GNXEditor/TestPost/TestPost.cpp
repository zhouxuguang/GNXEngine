//
//  TestPost.cpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/21.
//

#include "TestPost.hpp"
#include "Runtime/RenderSystem/include/PostProcess/PostProcessing.h"

static PostProcessing* postProcessing = nullptr;

void initPostResource(RenderDevicePtr renderDevice)
{
    postProcessing = new PostProcessing(renderDevice);
}

void testPost(const RenderEncoderPtr &renderEncoder, const RCTexturePtr texture)
{
    postProcessing->SetRenderTexture(texture);
    postProcessing->Process(renderEncoder);
}
