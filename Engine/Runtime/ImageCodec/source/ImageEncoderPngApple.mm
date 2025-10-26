//
//  image_encoder_png_apple.mm
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderPng.h"
#include "ImageEncoderApple.h"

#ifndef USE_PNG_LIB

NAMESPACE_IMAGECODEC_BEGIN

bool ImageEncoderPNG::onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const
{
    CGImageRef imageRef = imageFormVImage(image);
    return saveCGImageToBuffer(dataStream, imageRef, kPNG_Format, quality);
}

bool ImageEncoderPNG::onEncodeFile(const char* fileName, const VImage& image, int quality) const
{
    CGImageRef imageRef = imageFormVImage(image);
    return saveCGImageToFile(fileName, imageRef, kPNG_Format, quality);
}

NAMESPACE_IMAGECODEC_END

#endif
