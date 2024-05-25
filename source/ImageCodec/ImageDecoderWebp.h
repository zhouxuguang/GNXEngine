//
//  ImageDecoderWebp.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/12.
//

#ifndef RENDERCORE_IMAGEDECODER_WEBP_INCLUDE_HHFJKSDF
#define RENDERCORE_IMAGEDECODER_WEBP_INCLUDE_HHFJKSDF

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

class ImageDecoderWEBP : public ImageDecoderImpl
{
private:
    virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap);

    virtual bool IsFormat(const void* buffer, size_t size);

    virtual ImageStoreFormat GetFormat() const;
};

NAMESPACE_IMAGECODEC_END

#endif /* RENDERCORE_IMAGEDECODER_WEBP_INCLUDE_HHFJKSDF */
