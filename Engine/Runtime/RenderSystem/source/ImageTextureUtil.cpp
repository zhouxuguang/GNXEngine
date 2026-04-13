//
//  ImageTextureUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "ImageTextureUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include <ktx.h>
#include <cstring>

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

//=============================================================================
// 从 KTX 文件加载 2D 纹理
// 使用 ktx 库直接读取 KTX v1 格式，绕过 ImageDecoder（不支持 KTX）
// 支持 RG16F、RGBA8、RGBA32F 等格式，用于离线预计算资源的运行时加载
//=============================================================================

// GL 内部格式常量（ktx.h 不提供这些定义）
static const uint32_t KTX_GL_RG16F   = 0x822F;
static const uint32_t KTX_GL_RGBA8   = 0x8058;
static const uint32_t KTX_GL_RGBA32F = 0x8814;
static const uint32_t KTX_GL_R32F    = 0x822E;
static const uint32_t KTX_GL_RG32F   = 0x8230;
static const uint32_t KTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F;
static const uint32_t KTX_GL_COMPRESSED_RGBA_BPTC_UNORM         = 0x8E8C;
static const uint32_t KTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM   = 0x8E8D;
static const uint32_t KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT       = 0x83F0;
static const uint32_t KTX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT      = 0x8C4C;

// GL 内部格式 → 引擎 TextureFormat 映射
static TextureFormat ConvertGLInternalFormatToEngine(uint32_t glFormat)
{
    switch (glFormat)
    {
        case KTX_GL_RG16F:   return kTexFormatRG16Float;
        case KTX_GL_RGBA8:   return kTexFormatRGBA8;
        case KTX_GL_RGBA32F: return kTexFormatRGBA32Float;
        case KTX_GL_R32F:    return kTexFormatR32Float;
        case KTX_GL_RG32F:   return kTexFormatRG32Float;
        case KTX_GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return kTexFormatBC6H;
        case KTX_GL_COMPRESSED_RGBA_BPTC_UNORM:         return kTexFormatBC7_RGB;
        case KTX_GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:   return kTexFormatBC7_SRGB;
        case KTX_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:       return kTexFormatDXT1_RGB;
        case KTX_GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:      return kTexFormatDXT1_SRGB;
        default:
            break;
    }
    return kTexFormatInvalid;
}

RCTexture2DPtr ImageTextureUtil::LoadKTXTexture(const char* filename)
{
    if (!filename) return nullptr;

    // 1. 用 ktx 库从文件创建纹理对象
    ktxTexture* ktx = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx);
    if (result != KTX_SUCCESS || !ktx)
    {
        LOG_ERROR("Failed to load KTX file: %s (error %d)", filename, (int)result);
        return nullptr;
    }

    // 2. 检查基本属性
    if (ktx->numDimensions != 2 || ktx->numFaces != 1 || ktx->isArray != 0)
    {
        LOG_ERROR("LoadKTXTexture only supports non-array 2D textures: %s", filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    uint32_t width     = ktx->baseWidth;
    uint32_t height    = ktx->baseHeight;
    uint32_t mipLevels = ktx->numLevels;

    // 获取 GL 内部格式（仅支持 KTX v1，v1 的 glInternalformat 在结构体中直接可用）
    uint32_t glInternalFormat = 0;
    if (ktx->classId == ktxTexture1_c)
    {
        ktxTexture1* ktx1 = reinterpret_cast<ktxTexture1*>(ktx);
        glInternalFormat = ktx1->glInternalformat;
    }
    else
    {
        // KTX v2 使用 DFD 描述格式，暂时不支持
        LOG_ERROR("KTX v2 format not yet supported, please use KTX v1: %s", filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    LOG_INFO("Loading KTX: %s (%ux%u, %d mips, format=0x%X)",
             filename, width, height, mipLevels, glInternalFormat);

    // 3. 转换格式
    TextureFormat engineFormat = ConvertGLInternalFormatToEngine(glInternalFormat);
    if (engineFormat == kTexFormatInvalid)
    {
        LOG_ERROR("Unsupported KTX internal format: 0x%X in file: %s", glInternalFormat, filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    // 4. 创建 GPU 纹理
    RCTexture2DPtr texture = GetRenderDevice()->CreateTexture2D(engineFormat,
                    TextureUsage::TextureUsageShaderRead, width, height, mipLevels);

    if (!texture)
    {
        LOG_ERROR("Failed to create GPU texture for KTX file: %s", filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    // 5. 上传各 mipmap 层数据到 GPU
    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        ktx_size_t offset = 0;

        result = ktxTexture_GetImageOffset(ktx, level, 0, 0, &offset);
        if (result != KTX_SUCCESS)
        {
            LOG_ERROR("Failed to get image offset for mip %d in KTX: %s", level, filename);
            continue;
        }

        const uint8_t* imageData = ktxTexture_GetData(ktx) + offset;
        ktx_size_t imgSize = ktxTexture_GetImageSize(ktx, level);

        uint32_t mipWidth  = std::max(1u, width >> level);
        uint32_t mipHeight = std::max(1u, height >> level);

        Rect2D rect(0, 0, mipWidth, mipHeight);
        uint32_t bytesPerRow = static_cast<uint32_t>(imgSize) / mipHeight;
        if (bytesPerRow == 0) bytesPerRow = static_cast<uint32_t>(imgSize);

        texture->ReplaceRegion(rect, level, imageData, bytesPerRow);
    }

    ktxTexture_Destroy(ktx);

    LOG_INFO("KTX texture loaded: %s -> %dx%d (%d mips)", filename, width, height, mipLevels);
    return texture;
}

RCTextureCubePtr ImageTextureUtil::LoadKTXCubemapTexture(const char* filename)
{
    if (!filename) return nullptr;

    // 1. 用 ktx 库从文件创建纹理对象
    ktxTexture* ktx = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx);
    if (result != KTX_SUCCESS || !ktx)
    {
        LOG_ERROR("Failed to load KTX cubemap file: %s (error %d)", filename, (int)result);
        return nullptr;
    }

    // 2. 检查是否为 Cubemap
    if (ktx->numDimensions != 2 || ktx->numFaces != 6 || ktx->isArray != 0)
    {
        LOG_ERROR("LoadKTXCubemapTexture only supports cubemap (numFaces=6, 2D, non-array): %s (faces=%d)", filename, ktx->numFaces);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    uint32_t width     = ktx->baseWidth;
    uint32_t height    = ktx->baseHeight;
    uint32_t mipLevels = ktx->numLevels;

    // 3. 获取 GL 内部格式
    uint32_t glInternalFormat = 0;
    if (ktx->classId == ktxTexture1_c)
    {
        ktxTexture1* ktx1 = reinterpret_cast<ktxTexture1*>(ktx);
        glInternalFormat = ktx1->glInternalformat;
    }
    else
    {
        LOG_ERROR("KTX v2 format not yet supported for cubemap: %s", filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    // 4. 转换格式
    TextureFormat engineFormat = ConvertGLInternalFormatToEngine(glInternalFormat);
    if (engineFormat == kTexFormatInvalid)
    {
        LOG_ERROR("Unsupported KTX cubemap internal format: 0x%X in file: %s", glInternalFormat, filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    LOG_INFO("Loading KTX cubemap: %s (%ux%u, %d mips, format=0x%X)",
             filename, width, height, mipLevels, glInternalFormat);

    // 5. 创建 GPU Cubemap 纹理
    RCTextureCubePtr texture = GetRenderDevice()->CreateTextureCube(
        engineFormat, TextureUsage::TextureUsageShaderRead, width, height, mipLevels);

    if (!texture)
    {
        LOG_ERROR("Failed to create GPU cubemap texture for KTX file: %s", filename);
        ktxTexture_Destroy(ktx);
        return nullptr;
    }

    // 6. 上传各面各 mipmap 层数据到 GPU
    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        ktx_size_t imageSize = ktxTexture_GetImageSize(ktx, level);
        uint32_t mipWidth  = std::max(1u, width >> level);
        uint32_t mipHeight = std::max(1u, height >> level);
        uint32_t bytesPerRow = static_cast<uint32_t>(imageSize) / mipHeight;
        if (bytesPerRow == 0) bytesPerRow = static_cast<uint32_t>(imageSize);

        for (uint32_t face = 0; face < 6; ++face)
        {
            ktx_size_t offset = 0;
            result = ktxTexture_GetImageOffset(ktx, level, 0, face, &offset);
            if (result != KTX_SUCCESS)
            {
                LOG_ERROR("Failed to get image offset for mip %d face %d in KTX: %s", level, face, filename);
                continue;
            }

            const uint8_t* imageData = ktxTexture_GetData(ktx) + offset;
            texture->ReplaceRegion(Rect2D(0, 0, mipWidth, mipHeight), level, face,
                                   imageData, bytesPerRow, static_cast<uint32_t>(imageSize));
        }
    }

    ktxTexture_Destroy(ktx);

    LOG_INFO("KTX cubemap loaded: %s -> %dx%d (%d mips, 6 faces)", filename, width, height, mipLevels);
    return texture;
}

NS_RENDERSYSTEM_END
