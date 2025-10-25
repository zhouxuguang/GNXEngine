
//
// Created by Zhou,Xuguang on 2015/10/22.
//

#ifndef RENDERENGINE_IMAGELOADERHDR_INCLUDE_SDGDFJ
#define RENDERENGINE_IMAGELOADERHDR_INCLUDE_SDGDFJ

#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

// HDR格式的纹理解析
class ImageDecoderHDR : public ImageDecoderImpl
{
private:
	virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap);

	virtual bool IsFormat(const void* buffer, size_t size);

	virtual ImageStoreFormat GetFormat() const;
};

NAMESPACE_IMAGECODEC_END

#endif

