//
//  testSkybox.cpp
//  testNX
//
//  Created by zhouxuguang on 2021/6/14.
//

#include "testSkybox.h"
#include "RenderSystem/SkyBox.h"
#include "RenderSystem/RenderEngine.h"
#include "BaseLib/BaseLib.h"

RenderSystem::SkyBox* initSky(RenderDevicePtr renderDevice)
{
//    NSString *positive_x = [[NSBundle mainBundle] pathForResource:@"right" ofType:@"jpg"];
//    NSString *negative_x = [[NSBundle mainBundle] pathForResource:@"left" ofType:@"jpg"];
//    NSString *positive_y = [[NSBundle mainBundle] pathForResource:@"top" ofType:@"jpg"];
//    NSString *negative_y = [[NSBundle mainBundle] pathForResource:@"bottom" ofType:@"jpg"];
//    NSString *positive_z = [[NSBundle mainBundle] pathForResource:@"front" ofType:@"jpg"];
//    NSString *negative_z = [[NSBundle mainBundle] pathForResource:@"back" ofType:@"jpg"];
    
//    NSString *positive_x = [[NSBundle mainBundle] pathForResource:@"right" ofType:@"ktx"];
//    NSString *negative_x = [[NSBundle mainBundle] pathForResource:@"left" ofType:@"ktx"];
//    NSString *positive_y = [[NSBundle mainBundle] pathForResource:@"top" ofType:@"ktx"];
//    NSString *negative_y = [[NSBundle mainBundle] pathForResource:@"bottom" ofType:@"ktx"];
//    NSString *positive_z = [[NSBundle mainBundle] pathForResource:@"front" ofType:@"ktx"];
//    NSString *negative_z = [[NSBundle mainBundle] pathForResource:@"back" ofType:@"ktx"];
    
//    NSString *positive_x = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/right.jpg";
//    NSString *negative_x = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/left.jpg";
//    NSString *positive_y = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/top.jpg";
//    NSString *negative_y = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/bottom.jpg";
//    NSString *positive_z = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/front.jpg";
//    NSString *negative_z = @"/Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/skybox/back.jpg";
    
    std::string pathSplit = std::string(1, PATHSPLIT);
    std::string positive_x = getMediaDir() + "skybox" + pathSplit + "right.jpg";
    std::string negative_x = getMediaDir() + "skybox" + pathSplit + "left.jpg";
    std::string positive_y = getMediaDir() + "skybox" + pathSplit + "top.jpg";
    std::string negative_y = getMediaDir() + "skybox" + pathSplit + "bottom.jpg";
    std::string positive_z = getMediaDir() + "skybox" + pathSplit + "front.jpg";
    std::string negative_z = getMediaDir() + "skybox" + pathSplit + "back.jpg";
    
    return RenderSystem::SkyBox::create(renderDevice, positive_x.c_str(), negative_x.c_str(),
                                          positive_y.c_str(), negative_y.c_str(), positive_z.c_str(), negative_z.c_str());
}
