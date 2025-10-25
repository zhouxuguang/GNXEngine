//
//  image_encoder_jpeg_apple.mm
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderJpeg.h"
#include "ImageEncoderApple.h"

#ifndef USE_JPEG_LIB

NAMESPACE_IMAGECODEC_BEGIN

bool ImageEncoderJPEG::onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const
{
    CGImageRef imageRef = imageFormVImage(image);
    return saveCGImageToBuffer(dataStream, imageRef, kJPEG_Format, quality);
}

bool ImageEncoderJPEG::onEncodeFile(const char* fileName, const VImage& image, int quality) const
{
    CGImageRef imageRef = imageFormVImage(image);
    return saveCGImageToFile(fileName, imageRef, kJPEG_Format, quality);
}

NAMESPACE_IMAGECODEC_END

#endif
