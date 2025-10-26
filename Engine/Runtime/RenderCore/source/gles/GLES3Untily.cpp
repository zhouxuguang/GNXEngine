//
//  GLES3Untily.cpp
//  RenderEngine
//
//  Created by zhouxuguang on 2021/4/28.
//  Copyright © 2021 Zhou,Xuguang. All rights reserved.
//

#include "GLES3Untily.h"
#include "gl3stub.h"
#include "glesw.h"
#include "TextureFormat.h"

NAMESPACE_RENDERCORE_BEGIN

#ifndef ETC1_RGB8_OES
    #define ETC1_RGB8_OES 0x8D64
#endif

TransferFormatGLES30 GetTransferFormatGLES30(uint32_t internalFormat)
{
    switch (internalFormat)
    {
        case GL_RGBA32F:            return TransferFormatGLES30(GL_RGBA,            GL_FLOAT                            );
        case GL_RGBA32I:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_INT                                );
        case GL_RGBA32UI:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_UNSIGNED_INT                        );
        case GL_RGBA16F:            return TransferFormatGLES30(GL_RGBA,            GL_HALF_FLOAT                        );
        case GL_RGBA16I:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_SHORT                            );
        case GL_RGBA16UI:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_UNSIGNED_SHORT                    );
        case GL_RGBA8:                return TransferFormatGLES30(GL_RGBA,            GL_UNSIGNED_BYTE                    );
        case GL_RGBA8I:                return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_BYTE                                );
        case GL_RGBA8UI:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_UNSIGNED_BYTE                    );
        case GL_SRGB8_ALPHA8:        return TransferFormatGLES30(GL_RGBA,            GL_UNSIGNED_BYTE                    );
        case GL_RGB10_A2:            return TransferFormatGLES30(GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV        );
        case GL_RGB10_A2UI:            return TransferFormatGLES30(GL_RGBA_INTEGER,    GL_UNSIGNED_INT_2_10_10_10_REV        );
        case GL_RGBA4:                return TransferFormatGLES30(GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4            );
        case GL_RGB5_A1:            return TransferFormatGLES30(GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1            );
        case GL_RGBA8_SNORM:        return TransferFormatGLES30(GL_RGBA,            GL_BYTE                                );
        case GL_RGB8:                return TransferFormatGLES30(GL_RGB,                GL_UNSIGNED_BYTE                    );
        case GL_RGB565:                return TransferFormatGLES30(GL_RGB,                GL_UNSIGNED_SHORT_5_6_5                );
        case GL_R11F_G11F_B10F:        return TransferFormatGLES30(GL_RGB,                GL_UNSIGNED_INT_10F_11F_11F_REV        );
        case GL_RGB32F:                return TransferFormatGLES30(GL_RGB,                GL_FLOAT                            );
        case GL_RGB32I:                return TransferFormatGLES30(GL_RGB_INTEGER,        GL_INT                                );
        case GL_RGB32UI:            return TransferFormatGLES30(GL_RGB_INTEGER,        GL_UNSIGNED_INT                        );
        case GL_RGB16F:                return TransferFormatGLES30(GL_RGB,                GL_HALF_FLOAT                        );
        case GL_RGB16I:                return TransferFormatGLES30(GL_RGB_INTEGER,        GL_SHORT                            );
        case GL_RGB16UI:            return TransferFormatGLES30(GL_RGB_INTEGER,        GL_UNSIGNED_SHORT                    );
        case GL_RGB8_SNORM:            return TransferFormatGLES30(GL_RGB,                GL_BYTE                                );
        case GL_RGB8I:                return TransferFormatGLES30(GL_RGB_INTEGER,        GL_BYTE                                );
        case GL_RGB8UI:                return TransferFormatGLES30(GL_RGB_INTEGER,        GL_UNSIGNED_BYTE                    );
        case GL_SRGB8:                return TransferFormatGLES30(GL_RGB,                GL_UNSIGNED_BYTE                    );
        case GL_RGB9_E5:            return TransferFormatGLES30(GL_RGB,                GL_UNSIGNED_INT_5_9_9_9_REV            );
        case GL_RG32F:                return TransferFormatGLES30(GL_RG,                GL_FLOAT                            );
        case GL_RG32I:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_INT                                );
        case GL_RG32UI:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_UNSIGNED_INT                        );
        case GL_RG16F:                return TransferFormatGLES30(GL_RG,                GL_HALF_FLOAT                        );
        case GL_RG16I:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_SHORT                            );
        case GL_RG16UI:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_UNSIGNED_SHORT                    );
        case GL_RG8:                return TransferFormatGLES30(GL_RG,                GL_UNSIGNED_BYTE                    );
        case GL_RG8I:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_BYTE                                );
        case GL_RG8UI:                return TransferFormatGLES30(GL_RG_INTEGER,        GL_UNSIGNED_BYTE                    );
        case GL_RG8_SNORM:            return TransferFormatGLES30(GL_RG,                GL_BYTE                                );
        case GL_R32F:                return TransferFormatGLES30(GL_RED,                GL_FLOAT                            );
        case GL_R32I:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_INT                                );
        case GL_R32UI:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_UNSIGNED_INT                        );
        case GL_R16F:                return TransferFormatGLES30(GL_RED,                GL_HALF_FLOAT                        );
        case GL_R16I:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_SHORT                            );
        case GL_R16UI:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_UNSIGNED_SHORT                    );
        case GL_R8:                    return TransferFormatGLES30(GL_RED,                GL_UNSIGNED_BYTE                    );
        case GL_R8I:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_BYTE                                );
        case GL_R8UI:                return TransferFormatGLES30(GL_RED_INTEGER,        GL_UNSIGNED_BYTE                    );
        case GL_R8_SNORM:            return TransferFormatGLES30(GL_RED,                GL_BYTE                                );
        case GL_DEPTH_COMPONENT32F:    return TransferFormatGLES30(GL_DEPTH_COMPONENT,    GL_FLOAT                            );
        case GL_DEPTH_COMPONENT24:    return TransferFormatGLES30(GL_DEPTH_COMPONENT,    GL_UNSIGNED_INT                        );
        case GL_DEPTH_COMPONENT16:    return TransferFormatGLES30(GL_DEPTH_COMPONENT,    GL_UNSIGNED_SHORT                    );
        case GL_DEPTH32F_STENCIL8:    return TransferFormatGLES30(GL_DEPTH_STENCIL,    GL_FLOAT_32_UNSIGNED_INT_24_8_REV    );
        case GL_DEPTH24_STENCIL8:    return TransferFormatGLES30(GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8                );
        default:                    return TransferFormatGLES30(GL_NONE,            GL_NONE                                );
    }
}

TransferFormatGLES GetTransferFormatGLES(uint32_t textureFormat)
{
    switch (textureFormat)
    {
        case kTexFormatAlpha8:
            return TransferFormatGLES(GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE);
            
        case kTexFormatLuma:
            return TransferFormatGLES(GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE);
            
        case kTexFormatRGBA4444:
            return TransferFormatGLES(GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
            
        case kTexFormatRGBA5551:
            return TransferFormatGLES(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
            
        case kTexFormatRGB565:
            return TransferFormatGLES(GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);

        case kTexFormatAlphaLum16:
            return TransferFormatGLES(GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);

        case kTexFormatRGBA32:
            return TransferFormatGLES(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
            
        case kTexFormatRGB24:
            return TransferFormatGLES(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
         
        //srgb
        case kTexFormatSRGB8:
            return TransferFormatGLES(GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE);
            
        case kTexFormatSRGB8_ALPHA8:
            return TransferFormatGLES(GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
            
        case kTexFormatDepth16:
            return TransferFormatGLES(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT);
            
        case kTexFormatDepth24:
            return TransferFormatGLES(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
            
        case kTexFormatDepth32:
            return TransferFormatGLES(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
            
        case kTexFormatDepth32Float:
            return TransferFormatGLES(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
            
        case kTexFormatDepth24Stencil8:
            return TransferFormatGLES(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
            
        case kTexFormatDepth32FloatStencil8:
            return TransferFormatGLES(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
            
        case kTexFormatRGBA16Float:
            return TransferFormatGLES(GL_RGBA16F, GL_RGBA, GL_FLOAT);
            
        case kTexFormatRGBA32Float:
            return TransferFormatGLES(GL_RGBA32F, GL_RGBA, GL_FLOAT);
            
        //ETC2
        case kTexFormatEAC_R:
            return TransferFormatGLES(GL_COMPRESSED_R11_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatEAC_R_SIGNED:
            return TransferFormatGLES(GL_COMPRESSED_SIGNED_R11_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatEAC_RG:
            return TransferFormatGLES(GL_COMPRESSED_RG11_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatEAC_RG_SIGNED:
            return TransferFormatGLES(GL_COMPRESSED_SIGNED_RG11_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_RGB:
            return TransferFormatGLES(GL_COMPRESSED_RGB8_ETC2, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_SRGB:
            return TransferFormatGLES(GL_COMPRESSED_SRGB8_ETC2, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_RGBA1:
            return TransferFormatGLES(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_SRGBA1:
            return TransferFormatGLES(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_RGBA8:
            return TransferFormatGLES(GL_COMPRESSED_RGBA8_ETC2_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatETC2_SRGBA8:
            return TransferFormatGLES(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, GL_NONE, GL_NONE);
            
        case kTexFormatETC1_RGB:
            return TransferFormatGLES(ETC1_RGB8_OES, GL_NONE, GL_NONE);
            
        default:
            return TransferFormatGLES(GL_NONE, GL_NONE, GL_NONE);
    }
}

uint32_t GetDefaultFramebufferColorFormatGLES30(void)
{
    int    redBits        = 0;
    int greenBits    = 0;
    int    blueBits    = 0;
    int    alphaBits    = 0;

    glGetIntegerv(GL_RED_BITS,        &redBits);
    glGetIntegerv(GL_GREEN_BITS,    &greenBits);
    glGetIntegerv(GL_BLUE_BITS,        &blueBits);
    glGetIntegerv(GL_ALPHA_BITS,    &alphaBits);

#define PACK_FMT(R, G, B, A) (((R) << 24) | ((G) << 16) | ((B) << 8) | (A))

    // \note [pyry] This may not hold true on some implementations - best effort guess only.
    switch (PACK_FMT(redBits, greenBits, blueBits, alphaBits))
    {
        case PACK_FMT(8,8,8,8):        return GL_RGBA8;
        case PACK_FMT(8,8,8,0):        return GL_RGB8;
        case PACK_FMT(4,4,4,4):        return GL_RGBA4;
        case PACK_FMT(5,5,5,1):        return GL_RGB5_A1;
        case PACK_FMT(5,6,5,0):        return GL_RGB565;
        default:                    return GL_NONE;
    }

#undef PACK_FMT
}

uint32_t GetDefaultFramebufferDepthFormatGLES30(void)
{
    int depthBits = 0;
    int stencilBits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

    if (stencilBits > 0)
    {
        switch (depthBits)
        {
        case 32:    return GL_DEPTH32F_STENCIL8;
        case 24:    return GL_DEPTH24_STENCIL8;
        case 16:    return GL_DEPTH_COMPONENT16; // There's probably no such config?
        default:    return GL_NONE;
        }

    }
    else
    {
        switch (depthBits)
        {
        case 32:    return GL_DEPTH_COMPONENT32F;
        case 24:    return GL_DEPTH_COMPONENT24;
        case 16:    return GL_DEPTH_COMPONENT16;
        default:    return GL_NONE;
        }

    }
}

NAMESPACE_RENDERCORE_END

