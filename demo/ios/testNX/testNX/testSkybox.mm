//
//  testSkybox.cpp
//  testNX
//
//  Created by zhouxuguang on 2021/6/14.
//

#include "testSkybox.h"
#include "RenderSystem/SkyBox.h"
#include <Foundation/Foundation.h>

RenderSystem::SkyBox* skyBox = nullptr;

void initSky(RenderDevicePtr renderDevice)
{
//    NSString *positive_x = [[NSBundle mainBundle] pathForResource:@"right" ofType:@"jpg"];
//    NSString *negative_x = [[NSBundle mainBundle] pathForResource:@"left" ofType:@"jpg"];
//    NSString *positive_y = [[NSBundle mainBundle] pathForResource:@"top" ofType:@"jpg"];
//    NSString *negative_y = [[NSBundle mainBundle] pathForResource:@"bottom" ofType:@"jpg"];
//    NSString *positive_z = [[NSBundle mainBundle] pathForResource:@"front" ofType:@"jpg"];
//    NSString *negative_z = [[NSBundle mainBundle] pathForResource:@"back" ofType:@"jpg"];
    
    NSString *positive_x = [[NSBundle mainBundle] pathForResource:@"right" ofType:@"ktx"];
    NSString *negative_x = [[NSBundle mainBundle] pathForResource:@"left" ofType:@"ktx"];
    NSString *positive_y = [[NSBundle mainBundle] pathForResource:@"top" ofType:@"ktx"];
    NSString *negative_y = [[NSBundle mainBundle] pathForResource:@"bottom" ofType:@"ktx"];
    NSString *positive_z = [[NSBundle mainBundle] pathForResource:@"front" ofType:@"ktx"];
    NSString *negative_z = [[NSBundle mainBundle] pathForResource:@"back" ofType:@"ktx"];
    
    skyBox = RenderSystem::SkyBox::create(renderDevice, positive_x.UTF8String, negative_x.UTF8String,
                                          positive_y.UTF8String, negative_y.UTF8String, positive_z.UTF8String, negative_z.UTF8String);
}

void drawSky(const rendercore::RenderEncoderPtr &renderEncoder)
{
    skyBox->render(renderEncoder);
}
