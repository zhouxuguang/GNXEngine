//
//  ImageDecoderWebp.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/12.
//

#include "ImageDecoderWebp.h"
#include "libwebp/src/webp/decode.h"
#include "libwebp/src/webp/format_constants.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderWEBP::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
    if (!bitmap)
    {
        return false;
    }
    
//    WebPDecoderConfig decodeConfig;
//    WebPIDecoder* piDecoder = WebPIDecode((const uint8_t*)buffer, size, &decodeConfig);
//    if (nullptr == piDecoder)
//    {
//        return false;
//    }
//    WebPIDelete(piDecoder);
    
    //解析
    int width = 0;
    int height = 0;
    uint8_t* pRGBAData = WebPDecodeRGBA((const uint8_t*)buffer, size, &width, &height);
    bitmap->SetImageInfo(FORMAT_RGBA8, width, height, pRGBAData, WebPFree);
    bitmap->SetPremultipliedAlpha(false);
//    if (WebPIsPremultipliedMode(decodeConfig.output.colorspace))
//    {
//
//    }
    
    return true;
}

bool ImageDecoderWEBP::IsFormat(const void* buffer, size_t size)
{
    if (nullptr == buffer || size < RIFF_HEADER_SIZE)
    {
        return false;
    }
    
    uint8_t riffHeader[4] = {'R', 'I', 'F', 'F'};
    uint8_t webpHeader[4] = {'W', 'E', 'B', 'P'};
    
    const uint8_t *temp = (const uint8_t *)buffer;
    uint32_t fileSize = 0;
    memcpy(&fileSize, temp + 4, 4);
    
    if (0 == memcmp(riffHeader, temp, 4) && 0 == memcmp(webpHeader, temp + 8, 4) && fileSize == size - 8)
    {
        return true;
    }
    
    return false;
}

ImageStoreFormat ImageDecoderWEBP::GetFormat() const
{
    return kWEBP_Format;
}

NAMESPACE_IMAGECODEC_END
