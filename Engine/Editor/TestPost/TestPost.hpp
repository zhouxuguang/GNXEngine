//
//  TestPost.hpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/21.
//

#ifndef TestPost_hpp
#define TestPost_hpp

#include "Runtime/RenderSystem/include/PostProcess/PostProcessing.h"

using namespace RenderSystem;

void initPostResource(RenderDevicePtr renderDevice);

void testPost(const RenderEncoderPtr &renderEncoder, const RCTexturePtr texture);

#endif /* TestPost_hpp */
