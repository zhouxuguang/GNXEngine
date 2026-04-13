#include "DXTCompressor.h"

// Add a preprocessor definition and include goofy header
#define GOOFYTC_IMPLEMENTATION
#include "TextureProcess/goofy_tc.h"
#include "TextureCompressCommon.h"

NS_ASSETPROCESS_BEGIN

// DXT1数据压缩, input需要是rgba32的数据
static void CompressDXT1_ISPC(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
	rgba_surface srcData;
	srcData.width = width;
	srcData.height = height;
	srcData.stride = stride;
	srcData.ptr = (uint8_t*)input;

	rgba_surface fillboderData;
	fill_borders(&fillboderData, &srcData, 4, 4, 32);

	CompressBlocksBC1(&fillboderData, result);
	free(fillboderData.ptr);
}

void CompressDXT1(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
	if (width % 16 != 0 || height % 4 != 0)
	{
        return CompressDXT1_ISPC(result, input, width, height, stride);
	}

    goofy::compressDXT1(result, input, width, height, stride);
}

void CompressBC7(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
	rgba_surface srcData;
	srcData.width = width;
	srcData.height = height;
	srcData.stride = stride;
	srcData.ptr = (uint8_t*)input;

	rgba_surface fillboderData;
	fill_borders(&fillboderData, &srcData, 4, 4, 32);

    bc7_enc_settings settings;
    GetProfile_alpha_veryfast(&settings);
	CompressBlocksBC7(&fillboderData, result, &settings);
	free(fillboderData.ptr);
}

void CompressBC6H(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
	rgba_surface srcData;
	srcData.width = width;
	srcData.height = height;
	srcData.stride = stride;
	srcData.ptr = (uint8_t*)input;

	rgba_surface fillboderData;
	fill_borders(&fillboderData, &srcData, 4, 4, 64);  // BC6H: 64 bpp (RGBA16F)

	bc6h_enc_settings settings;
	GetProfile_bc6h_fast(&settings);
	CompressBlocksBC6H(&fillboderData, result, &settings);
	free(fillboderData.ptr);
}

NS_ASSETPROCESS_END
