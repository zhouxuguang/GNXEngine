#ifndef GNX_ENGINE_TEXYURE_DXTC_COMPRESSOR_INCLUDE_JDJ
#define GNX_ENGINE_TEXYURE_DXTC_COMPRESSOR_INCLUDE_JDJ

#include "AssetProcessDefine.h"

NS_ASSETPROCESS_BEGIN

// DXTC纹理压缩的接口

// DXT1数据压缩,input需要64字节对齐,并且是RGBA4个通道的数据
void ASSET_PROCESS_API CompressDXT1(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride);

// BC7数据压缩
void ASSET_PROCESS_API CompressBC7(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride);

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_TEXYURE_DXTC_COMPRESSOR_INCLUDE_JDJ

