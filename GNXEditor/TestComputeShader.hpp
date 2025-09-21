//
//  TestComputeShader.hpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef TestComputeShader_hpp
#define TestComputeShader_hpp

#include <stdio.h>
#include "RenderCore/GraphicsPipeline.h"
#include "RenderCore/ComputeEncoder.h"

USING_NS_RENDERCORE


void TestADD();

ComputePipelinePtr initTestimageGray();

void testImageGrayDraw(ComputeEncoderPtr computeEncoder, ComputePipelinePtr computePipeline,
                       RCTexturePtr inputTexture, RCTexturePtr outputTexture);



#endif /* TestComputeShader_hpp */
