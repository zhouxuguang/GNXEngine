//
//  image_encoder_jpeg.h
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef RENDERENGINE_IMAGEENCODER_JPEG_NJNFJNJ_INCLUDE_H
#define RENDERENGINE_IMAGEENCODER_JPEG_NJNFJNJ_INCLUDE_H

#include "VImage.h"
#include "ImageEncoder.h"

#ifdef __APPLE__
    //#define USE_JPEG_LIB //苹果平台下默认使用系统的库编码JPEG
#else
    #define USE_JPEG_LIB
#endif

NAMESPACE_IMAGECODEC_BEGIN

class ImageEncoderJPEG
{
public:
    bool onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const;
    
    bool onEncodeFile(const char* fileName, const VImage& image, int quality) const;
};

NAMESPACE_IMAGECODEC_END

#endif /* RENDERENGINE_IMAGEENCODER_JPEG_NJNFJNJ_INCLUDE_H */
