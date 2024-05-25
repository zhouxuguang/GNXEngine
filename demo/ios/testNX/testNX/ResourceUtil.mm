//
//  ResourceUtil.m
//  testNX
//
//  Created by zhouxuguang on 2021/5/9.
//

#import "ResourceUtil.h"
#import <UIKit/UIKit.h>

std::string getShaderPath()
{
    
    NSString *imagePath = [[NSBundle mainBundle] pathForResource:@"PositionTexture_frag" ofType:@"spv"];
    return std::string([imagePath UTF8String]);
}

std::string getTexturePath()
{
    
    NSString *imagePath = [[NSBundle mainBundle] pathForResource:@"tex_png" ofType:@"webp"];
    return std::string([imagePath UTF8String]);
}
