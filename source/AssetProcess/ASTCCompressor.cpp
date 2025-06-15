#include "ASTCCompressor.h"
#include "TextureProcess/ispc_texcomp.h"
#include "TextureCompressCommon.h"

NS_ASSETPROCESS_BEGIN

void CompressASTC(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height,
              uint32_t blockWidth, uint32_t blockHeight, uint32_t stride)
{
    rgba_surface srcData;
    srcData.width = width;
    srcData.height = height;
    srcData.stride = stride;
    srcData.ptr = (uint8_t*)input;
    
    rgba_surface fillboderData;
    fill_borders(&fillboderData, &srcData, blockWidth, blockHeight);
    
    astc_enc_settings astcSettings;
    GetProfile_astc_alpha_fast(&astcSettings, blockWidth, blockHeight);
    
    CompressBlocksASTC(&fillboderData, result, &astcSettings);
    free(fillboderData.ptr);
}

NS_ASSETPROCESS_END
