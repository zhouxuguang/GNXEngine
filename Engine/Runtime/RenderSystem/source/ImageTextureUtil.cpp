//
//  ImageTextureUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "ImageTextureUtil.h"
#include "UtilsCubemap.h"

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

int getNumMipMapLevels2D(int w, int h)
{
    int levels = 1;
    while ((w | h) >> levels)
        levels += 1;
    return levels;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

RCTextureCubePtr LoadEquirectangularMap(const std::string& fileName)
{
    int w, h, comp;
    float* img = stbi_loadf(fileName.c_str(), &w, &h, &comp, 4);
    assert(img);
    Bitmap in(w, h, 4, eBitmapFormat_Float, img);
    const bool isEquirectangular = w == 2 * h;
    Bitmap out = isEquirectangular ? convertEquirectangularMapToVerticalCross(in) : in;
    stbi_image_free(img);
    Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);
    
    RCTextureCubePtr cubeMapPtr = GetRenderDevice()->CreateTextureCube(kTexFormatRGBA32Float,
                                                TextureUsage::TextureUsageShaderRead, cubemap.w_, cubemap.h_, 1);
    
    const uint8_t* data = cubemap.data_.data();

    for (int i = 0; i != 6; ++i)
    {
        int imageSize = cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
        
        int face = (int)kCubeFacePX + i;
        Rect2D rect(0, 0, cubemap.w_, cubemap.h_);
        cubeMapPtr->ReplaceRegion(rect, 0, face, data,
                                  cubemap.w_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_), 0);
        data += cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
    }
    
    return cubeMapPtr;
}

NS_RENDERSYSTEM_END
