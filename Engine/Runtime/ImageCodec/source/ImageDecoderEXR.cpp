#include "ImageDecoderEXR.h"
#include "tinyexr/tinyexr.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderEXR::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
	if (!buffer || !size || !bitmap)
	{
		return false;
	}

	ImagePixelFormat imageFormat = FORMAT_RGBA32Float;

	float* outData = nullptr; // width * height * RGBA
	int width = 0;
	int height = 0;
	const char* err = nullptr;

	int ret = LoadEXRFromMemory(&outData, &width, &height, (const uint8_t*)buffer, size, &err);

	if (ret != TINYEXR_SUCCESS) 
	{
		if (err) 
		{
			FreeEXRErrorMessage(err); // release memory of error message.
		}
		free(outData);
		outData = nullptr;
	}

	bitmap->SetImageInfo(imageFormat, width, height, outData, ::free);
	return true;
}

bool ImageDecoderEXR::IsFormat(const void* buffer, size_t size)
{
	return TINYEXR_SUCCESS == IsEXRFromMemory((const uint8_t*)buffer, size);
}

imagecodec::ImageStoreFormat ImageDecoderEXR::GetFormat() const
{
	return kHDR_Format;
}

NAMESPACE_IMAGECODEC_END