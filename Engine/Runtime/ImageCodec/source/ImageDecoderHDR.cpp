#include "ImageDecoderHDR.h"
#include "libhdr/stb_image.h"

NAMESPACE_IMAGECODEC_BEGIN

static bool hdr_test_core(const uint8_t* buffer, const char* signature)
{
	for (int i = 0; signature[i]; ++i)
	{
		if (buffer[i] != signature[i])
		{
			return false;
		}
	}
	return true;
}

static bool hdr_test(const uint8_t* buffer)
{
	bool r = hdr_test_core(buffer, "#?RADIANCE\n");
	if (!r) 
	{
		r = hdr_test_core(buffer, "#?RGBE\n");
	}
	return r;
}

bool ImageDecoderHDR::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
	if (!buffer || !size || !bitmap)
	{
		return false;
	}

	int width = 0;
	int height = 0;
	int comp = 0;
	float* pData = hdr::stbi_loadf_from_memory((hdr::stbi_uc const*)buffer, size, &width, &height, &comp, 0);
	if (!pData)
	{
		return false;
	}

	ImagePixelFormat imageFormat = FORMAT_RGB32Float;
	if (4 == comp)
	{
		imageFormat = FORMAT_RGBA32Float;
	}

	bitmap->SetImageInfo(imageFormat, width, height, pData, hdr::stbi_image_free);
	return true;
}

bool ImageDecoderHDR::IsFormat(const void* buffer, size_t size)
{
	return hdr_test((const uint8_t*)buffer);
}

imagecodec::ImageStoreFormat ImageDecoderHDR::GetFormat() const
{
	return kHDR_Format;
}

NAMESPACE_IMAGECODEC_END