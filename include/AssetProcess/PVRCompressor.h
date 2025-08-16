#ifndef GNX_ENGINE_TEXYURE_PVRTEXCOMPRESSOR_INCLUDE_HNFJSDGJGJ
#define GNX_ENGINE_TEXYURE_PVRTEXCOMPRESSOR_INCLUDE_HNFJSDGJGJ

#include "AssetProcessDefine.h"

NS_ASSETPROCESS_BEGIN

// PVR数据压缩,input是RGBA4个通道的数据
void CompressPVRRGB4Bpp(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height);

// PVR数据压缩,input是RGBA4个通道的数据
void CompressPVRRGBA4Bpp(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height);

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_TEXYURE_PVRTEXCOMPRESSOR_INCLUDE_HNFJSDGJGJ
