//
// Created by Zhou,Xuguang on 2018/10/27.
//

#ifndef RENDERENGINE_IMAGEDECODERTGA_H
#define RENDERENGINE_IMAGEDECODERTGA_H

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

class ImageDecoderTGA : public ImageDecoderImpl
{
private:
    bool onDecode(const void* buffer, size_t size, VImage* bitmap) override;

    bool IsFormat(const void* buffer, size_t size) override;

    ImageStoreFormat GetFormat() const override;
};

ImageDecoderTGA* CreateTGADecoder();

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERTGA_H
