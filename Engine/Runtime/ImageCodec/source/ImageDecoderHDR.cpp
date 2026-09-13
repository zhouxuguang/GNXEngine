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
	// Radiance HDR 魔数最长 11 字节（"#?RADIANCE\n"）。
	// 必须先用 size 做长度校验：hdr_test_core 会一直读到签名末尾，
	// 缓冲区更短时（例如被截断的 .hdr 文件）会越界读取缓冲区之外的内存。
	const size_t kMinHeaderSize = 11;
	if (nullptr == buffer || size < kMinHeaderSize)
	{
		return false;
	}

	return hdr_test((const uint8_t*)buffer);
}

imagecodec::ImageStoreFormat ImageDecoderHDR::GetFormat() const
{
	return kHDR_Format;
}

NAMESPACE_IMAGECODEC_END