#include "DXTCompressor.h"

// Add a preprocessor definition and include goofy header
#define GOOFYTC_IMPLEMENTATION
#include "TextureProcess/goofy_tc.h"
#include "TextureCompressCommon.h"

NS_ASSETPROCESS_BEGIN

void CompressDXT1(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
    goofy::compressDXT1(result, input, width, height, stride);
}

void CompressDXT1_ISPC(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
    rgba_surface srcData;
    srcData.width = width;
    srcData.height = height;
    srcData.stride = stride;
    srcData.ptr = (uint8_t*)input;
    
    rgba_surface fillboderData;
    fill_borders(&fillboderData, &srcData, 4, 4);
    
    CompressBlocksBC1(&fillboderData, result);
    free(fillboderData.ptr);
}

void CompressBC7(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
	rgba_surface srcData;
	srcData.width = width;
	srcData.height = height;
	srcData.stride = stride;
	srcData.ptr = (uint8_t*)input;

	rgba_surface fillboderData;
	fill_borders(&fillboderData, &srcData, 4, 4);

    bc7_enc_settings settings;
    GetProfile_alpha_veryfast(&settings);
	CompressBlocksBC7(&fillboderData, result, &settings);
	free(fillboderData.ptr);
}

NS_ASSETPROCESS_END
