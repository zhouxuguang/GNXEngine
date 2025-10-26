//
//  image_encoder.cpp
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoder.h"
#include "ImageEncoderPng.h"
#include "ImageEncoderJpeg.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageEncoder::EncodeFile(const char *fileName, const VImage& image, ImageStoreFormat format, int quality)
{
    switch (format)
    {
        case kPNG_Format:
        {
            ImageEncoderPNG pngEncoder;
            return pngEncoder.onEncodeFile(fileName, image, quality);
        }
            
        case kJPEG_Format:
        {
            ImageEncoderJPEG jpegEncoder;
            return jpegEncoder.onEncodeFile(fileName, image, quality);
        }
            
        default:
            break;
    }
    return false;
}

bool ImageEncoder::EncodeMemory(std::vector<unsigned char>& dataStream, const VImage& image, ImageStoreFormat format, int quality)
{
    switch (format)
    {
        case kPNG_Format:
        {
            ImageEncoderPNG pngEncoder;
            return pngEncoder.onEncode(dataStream, image, quality);
        }

        case kJPEG_Format:
        {
            ImageEncoderJPEG jpegEncoder;
            return jpegEncoder.onEncode(dataStream, image, quality);
        }
            
        default:
            break;
    }
    return false;
}

NAMESPACE_IMAGECODEC_END
