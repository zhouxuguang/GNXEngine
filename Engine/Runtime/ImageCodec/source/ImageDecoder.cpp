//
// Created by Zhou,Xuguang on 2015/10/22.
//

#include "ImageDecoder.h"
#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoder::DecodeFile(const char *fileName, VImage *bitmap, ImageStoreFormat *format)
{
    return ImageDecoderImpl::DecodeFile(fileName, bitmap, format);

}

bool ImageDecoder::DecodeMemory(const void *buffer, size_t size, VImage *bitmap, ImageStoreFormat *format)
{
    return ImageDecoderImpl::DecodeMemory(buffer, size, bitmap, format);
}

NAMESPACE_IMAGECODEC_END
