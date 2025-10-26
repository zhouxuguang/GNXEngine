//
//  ResourceUtil.m
//  testNX
//
//  Created by zhouxuguang on 2021/5/9.
//

#import "ResourceUtil.h"

#if TARGET_OS_MAC
    #import <AppKit/AppKit.h>
#elseif TARGET_OS_IOS
    #import <UIKit/UIKit.h>
#endif



std::string getShaderPath()
{
    return "";
//    NSString *imagePath = [[NSBundle mainBundle] pathForResource:@"PositionTexture_frag" ofType:@"spv"];
//    return std::string([imagePath UTF8String]);
}

std::string getTexturePath()
{
    return "";
    
//    NSString *imagePath = [[NSBundle mainBundle] pathForResource:@"tex_png" ofType:@"webp"];
//    return std::string([imagePath UTF8String]);
}
