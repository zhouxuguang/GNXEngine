#ifndef GNX_ENGINE_TEXYURE_ASTCCOMPRESSOR_INCLUDESDMGKFM
#define GNX_ENGINE_TEXYURE_ASTCCOMPRESSOR_INCLUDESDMGKFM

#include "AssetProcessDefine.h"

NS_ASSETPROCESS_BEGIN

// ASTC纹理压缩的接口

void CompressASTC(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height,
              uint32_t blockWidth, uint32_t blockHeight, uint32_t stride);

// 多线程版本
void CompressASTC_MT(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height,
              uint32_t blockWidth, uint32_t blockHeight, uint32_t stride);

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_TEXYURE_ASTCCOMPRESSOR_INCLUDESDMGKFM
