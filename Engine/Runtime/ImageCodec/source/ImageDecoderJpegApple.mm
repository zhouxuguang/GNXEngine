//
//  image_decoder_jpeg_apple.mm
//  
//
//  Created by zhouxuguang on 2019/7/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#include "ImageDecoderJpeg.h"
#include "ImageDecoderApple.h"

NAMESPACE_IMAGECODEC_BEGIN

#ifndef USE_JPEG_LIB

bool ImageDecoderJPEG::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    return DecodeAppleImage(buffer, size, bitmap);
}

ImageStoreFormat ImageDecoderJPEG::GetFormat() const
{
    return kJPEG_Format;
}

bool ImageDecoderJPEG::IsFormat(const void *buffer, size_t size)
{
    if (size < 10)
    {
        return false;
    }
    
    const uint8_t JPG_SOI[] = {0xFF, 0xD8};
    
    bool bFlag = memcmp(buffer, JPG_SOI, 2) == 0;
    if (!bFlag)
    {
        return false;
    }
    
    uint8_t* pJpegData = (uint8_t*)buffer;
    
    const uint8_t JFIF[] = {0x4A, 0x46, 0x49, 0x46};
    const uint8_t Exif[] = {0x45, 0x78, 0x69, 0x66};
    
    //如果都不是JFIF和Exif，可以认为不是jpeg文件
    if (memcmp(pJpegData + 6, JFIF, 4) && memcmp(pJpegData + 6, Exif, 4))
    {
        return false;
    }
    
    //EOI 的检查
    const uint8_t EOI[] = {0xFF, 0xD9};
    if (memcmp(pJpegData + size - 2, EOI, 2))
    {
        return false;
    }
    
    return true;
}

#endif

NAMESPACE_IMAGECODEC_END
