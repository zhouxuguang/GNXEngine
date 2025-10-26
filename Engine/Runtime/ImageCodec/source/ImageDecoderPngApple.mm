//
//  image_decoder_png_apple.mm
//  
//
//  Created by zhouxuguang on 2019/7/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#include "ImageDecoderPng.h"
#include "ImageDecoderApple.h"

#ifndef USE_PNG_LIB

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderPNG::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    return DecodeAppleImage(buffer, size, bitmap);
}

ImageStoreFormat ImageDecoderPNG::GetFormat() const
{
    return kPNG_Format;
}

static int pngHeaderCheck(const unsigned char* sig, size_t start, size_t num_to_check)
{
    unsigned char png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    
    if (num_to_check > 8)
        num_to_check = 8;
    
    else if (num_to_check < 1)
        return (-1);
    
    if (start > 7)
        return (-1);
    
    if (start + num_to_check > 8)
        num_to_check = 8 - start;
    
    return ((int)(memcmp(&sig[start], &png_signature[start], num_to_check)));
}

bool ImageDecoderPNG::IsFormat(const void *buffer, size_t size)
{
    int is_png = pngHeaderCheck((const unsigned char*)buffer, 0, 8);
    return is_png == 0;
}

NAMESPACE_IMAGECODEC_END

#endif
