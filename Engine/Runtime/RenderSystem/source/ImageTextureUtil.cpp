//
//  ImageTextureUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "ImageTextureUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"

NS_RENDERSYSTEM_BEGIN

TextureDesc ImageTextureUtil::getTextureDescriptor(const VImage& image)
{
    TextureDesc textureDescriptor;
    switch (image.GetFormat())
    {
        case imagecodec::FORMAT_RGBA8:
            textureDescriptor.format = kTexFormatRGBA8;
            break;
            
        case imagecodec::FORMAT_RGB8:
            textureDescriptor.format = kTexFormatRGB24;
            break;
            
        case imagecodec::FORMAT_RGBA4444:
            textureDescriptor.format = kTexFormatRGBA4444;
            break;
            
        case imagecodec::FORMAT_R5G6B5:
            textureDescriptor.format = kTexFormatRGB565;
            break;
            
        case imagecodec::FORMAT_RGB5A1:
            textureDescriptor.format = kTexFormatRGBA5551;
            break;
            
        case imagecodec::FORMAT_GRAY8:
            textureDescriptor.format = kTexFormatLuma;
            break;
            
        case imagecodec::FORMAT_GRAY8_ALPHA8:
            textureDescriptor.format = kTexFormatAlphaLum16;
            break;
            
        //srgb
        case imagecodec::FORMAT_SRGB8:
            textureDescriptor.format = kTexFormatSRGB8;
            break;
            
        case imagecodec::FORMAT_SRGB8_ALPHA8:
            textureDescriptor.format = kTexFormatSRGB8_ALPHA8;
            break;
            
        default:
            break;
    }
    
    textureDescriptor.width = image.GetWidth();
    textureDescriptor.height = image.GetHeight();
    textureDescriptor.bytesPerRow = image.GetBytesPerRow();
    textureDescriptor.mipmaped = image.GetMipCount() > 1;
    
    return textureDescriptor;
}

RCTexture2DPtr ImageTextureUtil::TextureFromFile(const char *filename)
{
    if (filename == nullptr)
    {
        return nullptr;
    }
    
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    if (!imagecodec::ImageDecoder::DecodeFile(filename, image.get()))
    {
        return nullptr;
    }
    
    // RGB8/SRGB8 不支持 Vulkan OPTIMAL tiling，需要提前转换为 RGBA8/SRGBA8
    ImagePixelFormat srcFormat = image->GetFormat();
    if (srcFormat == imagecodec::FORMAT_RGB8 || srcFormat == imagecodec::FORMAT_SRGB8)
    {
        uint32_t width = image->GetWidth();
        uint32_t height = image->GetHeight();
        const uint8_t* srcData = image->GetImageData();
        uint32_t srcBytesPerRow = image->GetBytesPerRow();

        uint32_t dstBytesPerRow = width * 4;
        uint32_t dstSize = dstBytesPerRow * height;
        uint8_t* dstData = (uint8_t*)malloc(dstSize);

        for (uint32_t y = 0; y < height; ++y)
        {
            const uint8_t* srcRow = srcData + y * srcBytesPerRow;
            uint8_t* dstRow = dstData + y * dstBytesPerRow;
            for (uint32_t x = 0; x < width; ++x)
            {
                dstRow[x * 4 + 0] = srcRow[x * 3 + 0];
                dstRow[x * 4 + 1] = srcRow[x * 3 + 1];
                dstRow[x * 4 + 2] = srcRow[x * 3 + 2];
                dstRow[x * 4 + 3] = 255;
            }
        }

        ImagePixelFormat dstFormat = (srcFormat == imagecodec::FORMAT_SRGB8)
            ? imagecodec::FORMAT_SRGB8_ALPHA8 : imagecodec::FORMAT_RGBA8;
        image->SetImageInfo(dstFormat, width, height, dstData, free);
    }
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    textureDescriptor.mipmaped = true;
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

RCTexture2DPtr ImageTextureUtil::CreateDiffuseTexture(float r, float g, float b)
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = r * 255;
    pData[1] = g * 255;
    pData[2] = b * 255;
    pData[3] = 255;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

RCTexture2DPtr ImageTextureUtil::CreateMetalRoughTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 0;
    pData[1] = 128;
    pData[2] = 255;
    pData[3] = 255;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

RCTexture2DPtr ImageTextureUtil::CreateNormalTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 128;
    pData[1] = 128;
    pData[2] = 255;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

RCTexture2DPtr ImageTextureUtil::CreateEmmisveTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 0;
    pData[1] = 0;
    pData[2] = 0;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

RCTexture2DPtr ImageTextureUtil::CreateAOTexture()
{
    uint8_t *pData = (uint8_t*)malloc(4);
    pData[0] = 255;
    pData[1] = 0;
    pData[2] = 0;
    pData[3] = 0;
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(FORMAT_RGBA8, 1, 1, pData, free);
    
    TextureDesc textureDescriptor = RenderSystem::ImageTextureUtil::getTextureDescriptor(*image);
    //textureDescriptor.mipmaped = true;
    
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(textureDescriptor.format,
                    TextureUsage::TextureUsageShaderRead, image->GetWidth(), image->GetHeight(), 1);
    Rect2D rect(0, 0, image->GetWidth(), image->GetHeight());
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());
    return texture;
}

int getNumMipMapLevels2D(int w, int h)
{
    int levels = 1;
    while ((w | h) >> levels)
        levels += 1;
    return levels;
}

//=============================================================================
// 运行时生成 BRDF LUT 纹理（Split-Sum 近似预积分表）
// 输出 RG16Half 格式，R=scale G=bias
//=============================================================================

RCTexture2DPtr ImageTextureUtil::CreateBRDFLUTTexture(uint32_t imageSize, uint32_t samples)
{
    // 1. 调用 AssetProcess 的 BRDF LUT 生成器（输出 FORMAT_RG16Float 内存数据）
    imagecodec::VImagePtr brdfImage = AssetProcess::GenerateBRDFLUT(imageSize, samples);
    if (!brdfImage || !brdfImage->GetImageData())
    {
        LOG_ERROR("BRDF LUT generation failed");
        return nullptr;
    }

    // 2. 直接使用生成器的 RG16Half 数据，无需格式转换
    const uint8_t* imageData = brdfImage->GetImageData();
    uint32_t bytesPerRow = imageSize * 2 * sizeof(uint16_t); // RG × 2 bytes

    // 3. 创建 GPU 纹理（RG16Float 格式，无需 mipmap）
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(kTexFormatRG16Float,
                    TextureUsage::TextureUsageShaderRead, imageSize, imageSize, 1);

    Rect2D rect(0, 0, imageSize, imageSize);
    texture->ReplaceRegion(rect, 0, imageData, bytesPerRow);

    LOG_INFO("BRDF LUT texture created: %ux%u (%u samples), format=RG16Float", imageSize, imageSize, samples);
    return texture;
}

NS_RENDERSYSTEM_END
