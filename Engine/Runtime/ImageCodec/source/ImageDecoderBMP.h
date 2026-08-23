//
// Created by Zhou,Xuguang on 2018/11/4.
//

#ifndef RENDERENGINE_IMAGEDECODERBMP_H
#define RENDERENGINE_IMAGEDECODERBMP_H

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

class ImageDecoderBMP : public ImageDecoderImpl
{
private:
    bool onDecode(const void* buffer, size_t size, VImage* bitmap) override;

    bool IsFormat(const void* buffer, size_t size) override;

    ImageStoreFormat GetFormat() const override;

};

ImageDecoderBMP* CreateBMPDecoder();

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERBMP_H
