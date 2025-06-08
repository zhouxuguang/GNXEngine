#include "DXTCompressor.h"

// Add a preprocessor definition and include goofy header
#define GOOFYTC_IMPLEMENTATION
#include "TextureProcess/goofy_tc.h"

NS_ASSETPROCESS_BEGIN

void CompressDXT1(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
    goofy::compressDXT1(result, input, width, height, stride);
}

NS_ASSETPROCESS_END