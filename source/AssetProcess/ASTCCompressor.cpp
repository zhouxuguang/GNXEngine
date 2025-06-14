#include "ASTCCompressor.h"
#include "TextureProcess/ispc_texcomp.h"

NS_ASSETPROCESS_BEGIN

static int idiv_ceil(int n, int d)
{
    return (n + d - 1) / d;
}

static void alloc_image(rgba_surface* img, int width, int height)
{
    img->width = width;
    img->height = height;
    img->stride = img->width * 4;
    img->ptr = (uint8_t*)malloc(img->height * img->stride);
}

static void fill_borders(rgba_surface* dst, rgba_surface* src, int block_width, int block_height)
{
    int full_width = idiv_ceil(src->width, block_width) * block_width;
    int full_height = idiv_ceil(src->height, block_height) * block_height;
    alloc_image(dst, full_width, full_height);
    
    ReplicateBorders(dst, src, 0, 0, 32);
}

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
