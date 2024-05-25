//
//  MTLRenderDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_RENDERDEFINE_INCLUDE_H
#define GNX_ENGINE_RENDERDEFINE_INCLUDE_H

#include "RenderDefine.h"
#include "TextureFormat.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreFoundation/CoreFoundation.h>

NAMESPACE_RENDERCORE_BEGIN

inline MTLPixelFormat ConvertTextureFormatToMetal(uint32_t textureFormat)
{
    switch (textureFormat)
    {
        case kTexFormatAlpha8:
            return MTLPixelFormatA8Unorm;
            
        case kTexFormatLuma:
            return MTLPixelFormatA8Unorm;
            
        case kTexFormatRGBA4444:
            return MTLPixelFormatABGR4Unorm;
            
        case kTexFormatRGBA5551:
            return MTLPixelFormatBGR5A1Unorm;
            
        case kTexFormatRGB565:
            return MTLPixelFormatB5G6R5Unorm;

        case kTexFormatAlphaLum16:
            return MTLPixelFormatRG8Unorm;

        case kTexFormatRGBA32:
            return MTLPixelFormatRGBA8Unorm;
            
        case kTexFormatRGB24:
            return MTLPixelFormatRGBA8Unorm;
         
        //srgb
        case kTexFormatSRGB8:
            return MTLPixelFormatRGBA8Unorm_sRGB;
            
        case kTexFormatSRGB8_ALPHA8:
            return MTLPixelFormatRGBA8Unorm_sRGB;
            
        case kTexFormatDepth16:
            return MTLPixelFormatDepth16Unorm;
            
        case kTexFormatDepth24:
            return MTLPixelFormatInvalid;
            
        case kTexFormatDepth32:
            return MTLPixelFormatDepth32Float;
            
        case kTexFormatDepth32Float:
            return MTLPixelFormatDepth32Float;
            
        case kTexFormatDepth24Stencil8:
        {
            if (@available(macos 10.11, *)) 
            {
                return MTLPixelFormatDepth24Unorm_Stencil8;
            }
            else
            {
                return MTLPixelFormatInvalid;
            }
        }
            
        case kTexFormatDepth32FloatStencil8:
            return MTLPixelFormatDepth32Float_Stencil8;
            
        case kTexFormatRGBA16Float:
            return MTLPixelFormatRGBA16Float;
            
        case kTexFormatRGBA32Float:
            return MTLPixelFormatRGBA32Float;
            
        //ETC2
        case kTexFormatEAC_R:
            return MTLPixelFormatEAC_R11Unorm;
            
        case kTexFormatEAC_R_SIGNED:
            return MTLPixelFormatEAC_R11Snorm;
            
        case kTexFormatEAC_RG:
            return MTLPixelFormatEAC_RG11Unorm;
            
        case kTexFormatEAC_RG_SIGNED:
            return MTLPixelFormatEAC_RG11Snorm;
            
        case kTexFormatETC2_RGB:
            return MTLPixelFormatETC2_RGB8;
            
        case kTexFormatETC2_SRGB:
            return MTLPixelFormatETC2_RGB8_sRGB;
            
        case kTexFormatETC2_RGBA1:
            return MTLPixelFormatETC2_RGB8A1;
            
        case kTexFormatETC2_SRGBA1:
            return MTLPixelFormatETC2_RGB8A1_sRGB;
            
        case kTexFormatETC2_RGBA8:
            return MTLPixelFormatEAC_RGBA8;
            
        case kTexFormatETC2_SRGBA8:
            return MTLPixelFormatEAC_RGBA8_sRGB;
            
        case kTexFormatETC1_RGB:
            return MTLPixelFormatInvalid;
            
        default:
            return MTLPixelFormatInvalid;
    }
}

struct FrameBufferFormat
{
    std::vector<MTLPixelFormat> colorFormats;
    MTLPixelFormat depthFormat;
    MTLPixelFormat stencilFormat;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDERDEFINE_INCLUDE_H */
