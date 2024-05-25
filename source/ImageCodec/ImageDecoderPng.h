//
// Created by Zhou,Xuguang on 2015/10/22.
//

#ifndef RENDERENGINE_IMAGELOADERPNG_H
#define RENDERENGINE_IMAGELOADERPNG_H

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

#ifdef __APPLE__
    //#define USE_PNG_LIB    //苹果平台下默认使用系统的库解析PNG
#else
    #define USE_PNG_LIB
#endif

class ImageDecoderPNG : public ImageDecoderImpl
{
private:
    virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap);

    virtual bool IsFormat(const void* buffer, size_t size);

    virtual ImageStoreFormat GetFormat() const;
};

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGELOADERPNG_H
