//
//  ImageTextureUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "ImageTextureUtil.h"
#include "UtilsCubemap.h"

NS_RENDERSYSTEM_BEGIN

TextureDescriptor ImageTextureUtil::getTextureDescriptor(const VImage& image)
{
    TextureDescriptor textureDescriptor;
    switch (image.GetFormat())
    {
        case imagecodec::FORMAT_RGBA8:
            textureDescriptor.format = kTexFormatRGBA32;
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
            
        //ETC2
        case imagecodec::FORMAT_EAC_R:
            textureDescriptor.format = kTexFormatEAC_R;
            break;
            
        case imagecodec::FORMAT_EAC_R_SIGNED:
            textureDescriptor.format = kTexFormatEAC_R_SIGNED;
            break;
            
        case imagecodec::FORMAT_EAC_RG:
            textureDescriptor.format = kTexFormatEAC_RG;
            break;
            
        case imagecodec::FORMAT_EAC_RG_SIGNED:
            textureDescriptor.format = kTexFormatEAC_RG_SIGNED;
            break;
            
        case imagecodec::FORMAT_ETC2_RGB:
            textureDescriptor.format = kTexFormatETC2_RGB;
            break;
            
        case imagecodec::FORMAT_ETC2_SRGB:
            textureDescriptor.format = kTexFormatETC2_SRGB;
            break;
            
        case imagecodec::FORMAT_ETC2_RGBA1:
            textureDescriptor.format = kTexFormatETC2_RGBA1;
            break;
            
        case imagecodec::FORMAT_ETC2_SRGBA1:
            textureDescriptor.format = kTexFormatETC2_SRGBA1;
            break;
            
        case imagecodec::FORMAT_ETC2_RGBA8:
            textureDescriptor.format = kTexFormatETC2_RGBA8;
            break;
            
        case imagecodec::FORMAT_ETC2_SRGBA8:
            textureDescriptor.format = kTexFormatETC2_SRGBA8;
            break;
            
        case imagecodec::FORMAT_ETC1_RGB:
            textureDescriptor.format = kTexFormatETC1_RGB;
            break;
            
        //ASTC
            
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

TextureCubePtr LoadEquirectangularMap(const std::string& fileName)
{
    int w, h, comp;
    float* img = stbi_loadf(fileName.c_str(), &w, &h, &comp, 4);
    assert(img);
    Bitmap in(w, h, 4, eBitmapFormat_Float, img);
    const bool isEquirectangular = w == 2 * h;
    Bitmap out = isEquirectangular ? convertEquirectangularMapToVerticalCross(in) : in;
    stbi_image_free(img);
    Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

    const int numMipmaps = getNumMipMapLevels2D(cubemap.w_, cubemap.h_);

//    glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTextureParameteri(handle_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//    glTextureParameteri(handle_, GL_TEXTURE_BASE_LEVEL, 0);
//    glTextureParameteri(handle_, GL_TEXTURE_MAX_LEVEL, numMipmaps-1);
//    glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
//    glTextureStorage2D(handle_, numMipmaps, GL_RGB32F, cubemap.w_, cubemap.h_);
//    const uint8_t* data = cubemap.data_.data();
//
//    for (unsigned i = 0; i != 6; ++i)
//    {
//        glTextureSubImage3D(handle_, 0, 0, 0, i, cubemap.w_, cubemap.h_, 1, GL_RGB, GL_FLOAT, data);
//        data += cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
//    }
//
//    glGenerateTextureMipmap(handle_);
    
    std::vector<TextureDescriptor> cubeDes;
    TextureDescriptor des;
    des.width = cubemap.w_;
    des.height = cubemap.h_;
    des.format = kTexFormatRGBA32Float;
    des.bytesPerRow = cubemap.w_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
    des.mipmaped = true;
    cubeDes.push_back(des);
    cubeDes.push_back(des);
    cubeDes.push_back(des);
    cubeDes.push_back(des);
    cubeDes.push_back(des);
    cubeDes.push_back(des);
    
    TextureCubePtr cubeMapPtr = getRenderDevice()->createTextureCubeWithDescriptor(cubeDes);
    
    const uint8_t* data = cubemap.data_.data();

    for (int i = 0; i != 6; ++i)
    {
        //glTextureSubImage3D(handle_, 0, 0, 0, i, cubemap.w_, cubemap.h_, 1, GL_RGB, GL_FLOAT, data);
        int imageSize = cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
        
        int face = (int)kCubeFacePX + i;
        cubeMapPtr->setTextureData((CubemapFace)face, imageSize, data);
        data += cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
    }
    
    return cubeMapPtr;
}

NS_RENDERSYSTEM_END
