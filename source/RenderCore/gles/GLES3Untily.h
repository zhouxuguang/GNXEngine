//
//  GLES3Untily.h
//  RenderEngine
//
//  Created by zhouxuguang on 2021/4/28.
//  Copyright © 2021 Zhou,Xuguang. All rights reserved.
//

#ifndef GNX_ENGINE_GLES3_UNTILY_INCLUDE_HPP
#define GNX_ENGINE_GLES3_UNTILY_INCLUDE_HPP

#include "GLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN


class TransferFormatGLES30
{
public:
    uint32_t    format;
    uint32_t    dataType;

    TransferFormatGLES30(uint32_t format, uint32_t dataType)
        : format    (format)
        , dataType    (dataType)
    {
    }
};

class TransferFormatGLES
{
public:
    uint32_t    internalFormat;
    uint32_t    format;
    uint32_t    dataType;
    
    TransferFormatGLES()
    {
    }

    TransferFormatGLES(uint32_t internalFormat, uint32_t format, uint32_t dataType)
        : internalFormat(internalFormat)
        , format(format)
        , dataType(dataType)
    {
    }
};

TransferFormatGLES GetTransferFormatGLES(uint32_t textureFormat);

//// Map RenderTextureFormat to closest GL sized internal format.
//uint32_t GetColorFormatGLES30 (RenderTextureFormat format);
//
//// Get closest depth internal format.
//uint32_t GetDepthOnlyFormatGLES30 (DepthBufferFormat format);
//
//// Get closest depth&stencil internal format.
//uint32_t GetDepthStencilFormatGLES30 (DepthBufferFormat format);

// Get transfer (upload) format, dataType pair for internal format.
TransferFormatGLES30 GetTransferFormatGLES30(uint32_t internalFormat);

// Get default framebuffer (0) internal format (guess based on bits)
uint32_t GetDefaultFramebufferColorFormatGLES30(void);

// Get default framebuffer (0) depth format
uint32_t GetDefaultFramebufferDepthFormatGLES30(void);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLES3_UNTILY_INCLUDE_HPP */
