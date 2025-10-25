//
//  ImageDecoderKTX.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/14.
//

#ifndef ImageDecoderKTX_hpp
#define ImageDecoderKTX_hpp

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN


class ImageDecoderKTX : public ImageDecoderImpl
{
private:
    virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap);

    virtual bool IsFormat(const void* buffer, size_t size);

    virtual ImageStoreFormat GetFormat() const;
};

NAMESPACE_IMAGECODEC_END

#endif /* ImageDecoderKTX_hpp */
