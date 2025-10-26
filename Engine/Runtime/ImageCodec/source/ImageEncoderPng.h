//
//  image_encoder_png.h
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef RENDERENGINE_IMAGE_ENCODER_PNG_JFSFHHF_H
#define RENDERENGINE_IMAGE_ENCODER_PNG_JFSFHHF_H

#include "VImage.h"
#include "ImageEncoder.h"

#ifdef __APPLE__
    //#define USE_PNG_LIB //苹果平台下默认使用系统的库编码PNG
#else
    #define USE_PNG_LIB
#endif

NAMESPACE_IMAGECODEC_BEGIN

class ImageEncoderPNG
{
public:
    bool onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const;
    
    bool onEncodeFile(const char* fileName, const VImage& image, int quality) const;
};

NAMESPACE_IMAGECODEC_END

#endif /* RENDERENGINE_IMAGE_ENCODER_PNG_JFSFHHF_H */
