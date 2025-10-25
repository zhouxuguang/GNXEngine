//
//  color_conveter.h 颜色格式转换工具类
//  GNXEngine
//
//  Created by zhouxuguang on 2019/9/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#ifndef IMAGE_COLOR_CONVERTER_INCLUDE_H
#define IMAGE_COLOR_CONVERTER_INCLUDE_H

#include "VImage.h"
#include <memory>

NAMESPACE_IMAGECODEC_BEGIN

class ColorConverter
{
public:
    //rgba32 -> rgb565
    static void convert_RGBA32toRGB565(const void* sP, unsigned int sN, void* dP);
    
    static void convert_RGBA32toRGB565(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage);
    
    //rgba32 -> rgba5551
    static void convert_RGBA32toRGBA5551(const void* sP, uint32_t sN, void* dP);
    
    //rgba32 -> rgba4444
    static void convert_RGBA32toRGBA4444(const void* sP, uint32_t sN, void* dP);
    
    //rgba24 -> rgb565
    static void convert_RGB24toRGB565(const void* sP, unsigned int sN, void* dP);
    
    static bool convert_RGB24toRGB565(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage);
    
    //rgba24 -> rgba5551
    static void convert_RGB24toRGBA5551(const void* sP, uint32_t sN, void* dP);
    
    //rgba24 -> rgba4444
    static void convert_RGB24toRGBA4444(const void* sP, uint32_t sN, void* dP);
    
    //grayAlpha16 -> rgba32
    static void convert_GrayAlpha16toRGBA32(const void* sp, uint32_t sN, void* dP);
    
    //rgb24 -> rgba32
    static void convert_RGB24toRGBA32(const void* sP, unsigned int sN, void* dP);
    
    static void convert_RGB24toRGBA32(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage);
};

NAMESPACE_IMAGECODEC_END

#endif /* IMAGE_COLOR_CONVERTER_INCLUDE_H */
