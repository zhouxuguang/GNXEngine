#include "ImageDecoderEXR.h"
#include "tinyexr/tinyexr.h"
#include <cstring>
#include <cstdlib>

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderEXR::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
	if (!buffer || !size || !bitmap)
	{
		return false;
	}

	const char* err = nullptr;

	// 1. Parse EXR version
	EXRVersion exr_version;
	int ret = ParseEXRVersionFromMemory(&exr_version, (const uint8_t*)buffer, size);
	if (ret != TINYEXR_SUCCESS)
	{
		return false;
	}

	// 2. Parse EXR header to get channel info
	EXRHeader exr_header;
	InitEXRHeader(&exr_header);
	ret = ParseEXRHeaderFromMemory(&exr_header, &exr_version, (const uint8_t*)buffer, size, &err);
	if (ret != TINYEXR_SUCCESS)
	{
		if (err) FreeEXRErrorMessage(err);
		return false;
	}

	// 3. Request FLOAT output for HALF channels
	for (int i = 0; i < exr_header.num_channels; i++)
	{
		if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF)
		{
			exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}

	// 4. Load EXR image
	EXRImage exr_image;
	InitEXRImage(&exr_image);
	ret = LoadEXRImageFromMemory(&exr_image, &exr_header, (const uint8_t*)buffer, size, &err);
	if (ret != TINYEXR_SUCCESS)
	{
		if (err) FreeEXRErrorMessage(err);
		FreeEXRHeader(&exr_header);
		return false;
	}

	// 5. Find channel indices by name
	int idxR = -1, idxG = -1, idxB = -1, idxA = -1;
	for (int c = 0; c < exr_header.num_channels; c++)
	{
		if (strcmp(exr_header.channels[c].name, "R") == 0) idxR = c;
		else if (strcmp(exr_header.channels[c].name, "G") == 0) idxG = c;
		else if (strcmp(exr_header.channels[c].name, "B") == 0) idxB = c;
		else if (strcmp(exr_header.channels[c].name, "A") == 0) idxA = c;
	}

	int width = exr_image.width;
	int height = exr_image.height;
	size_t pixel_count = (size_t)width * height;

	float** images = reinterpret_cast<float**>(exr_image.images);

	// 6. Determine output format and allocate memory based on actual channels
	ImagePixelFormat format;
	int num_channels_out;
	float* outData = nullptr;

	if (idxR >= 0 && idxG >= 0 && idxB >= 0 && idxA >= 0)
	{
		// RGBA - 4 channels
		format = FORMAT_RGBA32Float;
		num_channels_out = 4;
		outData = (float*)malloc(sizeof(float) * 4 * pixel_count);
		for (size_t i = 0; i < pixel_count; i++)
		{
			outData[i * 4 + 0] = images[idxR][i];
			outData[i * 4 + 1] = images[idxG][i];
			outData[i * 4 + 2] = images[idxB][i];
			outData[i * 4 + 3] = images[idxA][i];
		}
	}
	else if (idxR >= 0 && idxG >= 0 && idxB >= 0)
	{
		// RGB only - 3 channels
		format = FORMAT_RGB32Float;
		num_channels_out = 3;
		outData = (float*)malloc(sizeof(float) * 3 * pixel_count);
		for (size_t i = 0; i < pixel_count; i++)
		{
			outData[i * 3 + 0] = images[idxR][i];
			outData[i * 3 + 1] = images[idxG][i];
			outData[i * 3 + 2] = images[idxB][i];
		}
	}
	else if (exr_header.num_channels == 1)
	{
		// Single channel (grayscale) - output as RGB
		format = FORMAT_GRAY8;
		num_channels_out = 3;
		outData = (float*)malloc(sizeof(float) * pixel_count);
		for (size_t i = 0; i < pixel_count; i++)
		{
			outData[i] = images[0][i];
		}
	}
	else
	{
		// Unsupported channel configuration
		FreeEXRImage(&exr_image);
		FreeEXRHeader(&exr_header);
		return false;
	}

	// 7. Cleanup EXR resources
	FreeEXRImage(&exr_image);
	FreeEXRHeader(&exr_header);

	bitmap->SetImageInfo(format, width, height, outData, ::free);
	return true;
}

bool ImageDecoderEXR::IsFormat(const void* buffer, size_t size)
{
	return TINYEXR_SUCCESS == IsEXRFromMemory((const uint8_t*)buffer, size);
}

imagecodec::ImageStoreFormat ImageDecoderEXR::GetFormat() const
{
	return kEXR_Format;
}

NAMESPACE_IMAGECODEC_END