#include "PVRCompressor.h"
#include "RgbBitmap.h"
#include "RgbaBitmap.h"
#include "PvrTcEncoder.h"

NS_ASSETPROCESS_BEGIN

// PVR数据压缩,input是RGBA4个通道的数据
void CompressPVRRGB4Bpp(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
    Javelin::RgbaBitmap bmp;
    bmp.width  = width;
    bmp.height = height;
    bmp.data   = const_cast<uint8_t*>(input);
    Javelin::PvrTcEncoder::EncodeRgb4Bpp(result, bmp);
    bmp.data = NULL;
}

// PVR数据压缩,input是RGBA4个通道的数据
void CompressPVRRGBA4Bpp(uint8_t* result, const uint8_t* input, uint32_t width, uint32_t height, uint32_t stride)
{
    Javelin::RgbaBitmap bmp;
    bmp.width  = width;
    bmp.height = height;
    bmp.data   = const_cast<uint8_t*>(input);
    Javelin::PvrTcEncoder::EncodeRgba4Bpp(result, bmp);
    bmp.data = NULL;
}

NS_ASSETPROCESS_END
