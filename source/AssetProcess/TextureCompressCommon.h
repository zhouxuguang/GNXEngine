#ifndef GNX_ENGINE_TEXYURE_COMPRESS_COMMON_INCLUDE_KSJDJF
#define GNX_ENGINE_TEXYURE_COMPRESS_COMMON_INCLUDE_KSJDJF

#include "AssetProcessDefine.h"
#include "TextureProcess/ispc_texcomp.h"

NS_ASSETPROCESS_BEGIN

inline uint32_t idiv_ceil(uint32_t n, uint32_t d)
{
    return (n + d - 1) / d;
}

inline void alloc_image(rgba_surface* img, uint32_t width, uint32_t height)
{
    img->width = width;
    img->height = height;
    img->stride = img->width * 4;
    img->ptr = (uint8_t*)malloc(img->height * img->stride);
}

inline void fill_borders(rgba_surface* dst, rgba_surface* src, uint32_t block_width, uint32_t block_height)
{
    uint32_t full_width = idiv_ceil(src->width, block_width) * block_width;
    uint32_t full_height = idiv_ceil(src->height, block_height) * block_height;
    alloc_image(dst, full_width, full_height);
    
    ReplicateBorders(dst, src, 0, 0, 32);
}

NS_ASSETPROCESS_END

#endif
