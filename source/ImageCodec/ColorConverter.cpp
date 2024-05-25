//
//  color_conveter.cpp
//  
//
//  Created by zhouxuguang on 2019/9/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#include "ColorConverter.h"
#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif

NAMESPACE_IMAGECODEC_BEGIN

//rgba32 -> rgb565
void ColorConverter::convert_RGBA32toRGB565(const void* sP, unsigned int sN, void* dP)
{
    uint8_t * sB = (uint8_t *)sP;
    uint16_t * dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t r = sB[0] >> 3;
        uint32_t g = sB[1] >> 2;
        uint32_t b = sB[2] >> 3;
        
        dB[0] = (r << 11) | (g << 5) | (b);
        
        sB += 4;
        dB += 1;
    }
}

void ColorConverter::convert_RGBA32toRGB565(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage)
{
#ifdef __APPLE__
    if (__builtin_available(iOS 7, *))
    {
        vImage_Buffer srcBuffer;
        srcBuffer.data = srcImage->GetPixels();
        srcBuffer.width = srcImage->GetWidth();
        srcBuffer.height = srcImage->GetHeight();
        srcBuffer.rowBytes = srcImage->GetBytesPerRow();
        
        vImage_Buffer dstBuffer;
        dstBuffer.data = dstImage->GetPixels();
        dstBuffer.width = dstImage->GetWidth();
        dstBuffer.height = dstImage->GetHeight();
        dstBuffer.rowBytes = dstImage->GetBytesPerRow();
        
        vImageConvert_RGBA8888toRGB565(&srcBuffer, &dstBuffer, kvImageDoNotTile);
    }
    
    else
    {
        uint32_t width = srcImage->GetWidth();
        uint32_t height = srcImage->GetHeight();
        convert_RGBA32toRGB565(srcImage->GetPixels(), width * height, dstImage->GetPixels());
    }
#else
    uint32_t width = srcImage->GetWidth();
    uint32_t height = srcImage->GetHeight();
    convert_RGBA32toRGB565(srcImage->GetPixels(), width * height, dstImage->GetPixels());
#endif
}

//rgba32 -> rgba5551
void ColorConverter::convert_RGBA32toRGBA5551(const void* sP, uint32_t sN, void* dP)
{
    uint8_t * sB = (uint8_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t r = sB[0] >> 3;
        uint32_t g = sB[1] >> 3;
        uint32_t b = sB[2] >> 3;
        uint32_t a = sB[3] >> 3;
        
        dB[0] = (a << 15) | (r << 10) | (g << 5) | (b);
        
        sB += 4;
        dB += 1;
    }
}

//rgba32 -> rgba4444
void ColorConverter::convert_RGBA32toRGBA4444(const void* sP, uint32_t sN, void* dP)
{
    uint32_t * sB = (uint32_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t pel = sB[x];
        // 解码数据
        uint32_t r = (pel << 8)  & 0xf000;
        uint32_t g = (pel >> 4) & 0x0f00;
        uint32_t b = (pel >> 16) & 0x00f0;
        uint32_t a = (pel >> 28) & 0x000f;
        
        // 打包
        dB[x] = r | g | b | a;
    }
}

//rgba24 -> rgb565
void ColorConverter::convert_RGB24toRGB565(const void* sP, unsigned int sN, void* dP)
{
    uint8_t * sB = (uint8_t *)sP;
    uint16_t * dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t r = sB[0] >> 3;
        uint32_t g = sB[1] >> 2;
        uint32_t b = sB[2] >> 3;
        
        dB[0] = (r << 11) | (g << 5) | (b);
        
        sB += 3;
        dB += 1;
    }
}

bool ColorConverter::convert_RGB24toRGB565(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage)
{
    if (!srcImage || !dstImage)
    {
        return false;
    }
    
    //转换为565
    uint32_t width = srcImage->GetWidth();
    uint32_t height = srcImage->GetHeight();
    dstImage->SetImageInfo(FORMAT_R5G6B5, width, height);
    dstImage->AllocPixels();
    
    convert_RGB24toRGB565(srcImage->GetPixels(), width * height, dstImage->GetPixels());
    return true;
}

//rgba24 -> rgba5551
void ColorConverter::convert_RGB24toRGBA5551(const void* sP, uint32_t sN, void* dP)
{
    uint8_t * sB = (uint8_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t r = sB[0] >> 3;
        uint32_t g = sB[1] >> 3;
        uint32_t b = sB[2] >> 3;
        uint32_t a = 255;
        
        dB[0] = (a << 15) | (r << 10) | (g << 5) | (b);
        
        sB += 3;
        dB += 1;
    }
}

void ColorConverter::convert_RGB24toRGBA4444(const void* sP, uint32_t sN, void* dP)
{
    uint32_t * sB = (uint32_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    
    for (uint32_t x = 0; x < sN; ++x)
    {
        uint32_t pel = sB[x];
        // 解码数据
        uint32_t r = (pel << 8)  & 0xf000;
        uint32_t g = (pel >> 4) & 0x0f00;
        uint32_t b = (pel >> 16) & 0x00f0;
        uint32_t a = 255 & 0x000f;
        
        // 打包
        dB[x] = r | g | b | a;
    }
}

void ColorConverter::convert_GrayAlpha16toRGBA32(const void* sp, uint32_t sN, void* dP)
{
    uint8_t* sB = (uint8_t*)sp;
    uint32_t* dB = (uint32_t*)dP;
    
    for (uint32_t i = 0; i < sN; ++i)
    {
        uint32_t gray = (uint32_t)sB[0];
        uint32_t aplha = (uint32_t)sB[1];
        dB[0] = aplha<<24 | gray << 16 | gray << 8 | gray;
        sB += 2;
        dB += 1;
    }
}

void ColorConverter::convert_RGB24toRGBA32(const void* sP, unsigned int sN, void* dP)
{
#ifdef __APPLE__
    vImage_Buffer srcBuffer;
    srcBuffer.data = (void*)sP;
    srcBuffer.width = sN;
    srcBuffer.height = 1;
    srcBuffer.rowBytes = sN * 3;
    
    vImage_Buffer dstBuffer;
    dstBuffer.data = dP;
    dstBuffer.width = sN;
    dstBuffer.height = 1;
    dstBuffer.rowBytes = sN * 4;
    vImageConvert_RGB888toRGBA8888(&srcBuffer, nullptr, 255, &dstBuffer, true, kvImageDoNotTile);
#else
#endif
}

void ColorConverter::convert_RGB24toRGBA32(const std::shared_ptr<VImage>& srcImage, std::shared_ptr<VImage> &dstImage)
{
    convert_RGB24toRGBA32(srcImage->GetPixels(), srcImage->GetWidth() * srcImage->GetHeight(), dstImage->GetPixels());
}

NAMESPACE_IMAGECODEC_END
