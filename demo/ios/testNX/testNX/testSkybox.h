//
//  testSkybox.hpp
//  testNX
//
//  Created by zhouxuguang on 2021/6/14.
//

#ifndef testSkybox_hpp
#define testSkybox_hpp

#include <stdio.h>
#include "RenderCore/RenderDevice.h"

void initSky(rendercore::RenderDevicePtr renderDevice);

void drawSky(const rendercore::RenderEncoderPtr &renderEncoder);

#endif /* testSkybox_hpp */
