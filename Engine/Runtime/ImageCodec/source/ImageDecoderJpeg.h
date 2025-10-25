//
// Created by Zhou,Xuguang on 2015/11/2.
//

#ifndef RENDERENGINE_IMAGEDECODERJPEG_H
#define RENDERENGINE_IMAGEDECODERJPEG_H

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

#ifdef __APPLE__
    //#define USE_JPEG_LIB    //苹果平台下默认使用系统的库解析JPEG
#else
    #define USE_JPEG_LIB
#endif


class ImageDecoderJPEG : public ImageDecoderImpl
{
private:
    virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap);

    virtual bool IsFormat(const void* buffer, size_t size);

    virtual ImageStoreFormat GetFormat() const;
};

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERJPEG_H
