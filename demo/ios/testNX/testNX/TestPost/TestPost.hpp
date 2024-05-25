//
//  TestPost.hpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/21.
//

#ifndef TestPost_hpp
#define TestPost_hpp

#include "RenderSystem/PostProcess/PostProcessing.h"

using namespace RenderSystem;

void initPostResource(rendercore::RenderDevicePtr renderDevice);

void testPost(const rendercore::RenderEncoderPtr &renderEncoder, const rendercore::RenderTexturePtr texture);

#endif /* TestPost_hpp */
