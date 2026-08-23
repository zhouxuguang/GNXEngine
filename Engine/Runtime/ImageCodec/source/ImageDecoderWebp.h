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
    bool onDecode(const void* buffer, size_t size, VImage* bitmap) override;

    bool IsFormat(const void* buffer, size_t size) override;

    ImageStoreFormat GetFormat() const override;
};

NAMESPACE_IMAGECODEC_END

#endif /* RENDERCORE_IMAGEDECODER_WEBP_INCLUDE_HHFJKSDF */
