//
// Created by Zhou,Xuguang on 2015/11/2.
//

#ifndef GNXENGINE_IMAGECODEC_IMAGEDECODERJPEG_H
#define GNXENGINE_IMAGECODEC_IMAGEDECODERJPEG_H

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

#ifdef __APPLE__
    //#define USE_JPEG_LIB
#else
    #define USE_JPEG_LIB
#endif


class ImageDecoderJPEG : public ImageDecoderImpl
{
private:
    bool onDecode(const void* buffer, size_t size, VImage* bitmap) override;

    bool IsFormat(const void* buffer, size_t size) override;

    ImageStoreFormat GetFormat() const override;
};

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERJPEG_H
