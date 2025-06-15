#include "ASTCCompressor.h"
#include "TextureProcess/ispc_texcomp.h"
#include "TextureCompressCommon.h"
#include "tbb/parallel_for.h"

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

void CompressASTC_MT(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height,
              uint32_t blockWidth, uint32_t blockHeight, uint32_t stride)
{
    // 将原始数据对齐到整除块大小的整数倍
    rgba_surface srcData;
    srcData.width = width;
    srcData.height = height;
    srcData.stride = stride;
    srcData.ptr = (uint8_t*)input;
    
    astc_enc_settings astcSettings;
    GetProfile_astc_alpha_fast(&astcSettings, blockWidth, blockHeight);
    
    uint32_t xBlockCount = idiv_ceil(width, blockWidth);
    uint32_t yBlockCount = idiv_ceil(height, blockHeight);
    uint64_t blockCount = xBlockCount * yBlockCount;
    
    tbb::parallel_for(tbb::blocked_range<size_t>(0, yBlockCount),
    [&] (tbb::blocked_range<size_t> r)
    {
        for (size_t i = r.begin(); i < r.end(); i++) 
        {
            // 当前块的原始图像编号
            uint32_t x = 0;
            uint32_t y = i * blockHeight;
            rgba_surface blockData;
            blockData.width = blockWidth * xBlockCount;
            blockData.height = blockHeight;
            blockData.stride = blockData.width * 4;
            blockData.ptr = (uint8_t*)malloc(blockData.height * blockData.stride);
            ReplicateBorders(&blockData, &srcData, x, y, 32);
            
            CompressBlocksASTC(&blockData, result + (i * xBlockCount * 16), &astcSettings);
            free(blockData.ptr);
        }
    });
}

NS_ASSETPROCESS_END
