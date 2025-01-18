#include "ImageDecoderHDR.h"
#include "libhdr/stb_image.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderHDR::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
	return true;
}

bool ImageDecoderHDR::IsFormat(const void* buffer, size_t size)
{
	return true;
}

imagecodec::ImageStoreFormat ImageDecoderHDR::GetFormat() const
{
	return kHDR_Format;
}

NAMESPACE_IMAGECODEC_END