//
//  MTLTextureBase.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLTextureBase.h"

NAMESPACE_RENDERCORE_BEGIN

MTLTextureBase::MTLTextureBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
{
    //command需要产生mipmap的纹理
    mCommandQueue = commandQueue;
    mDevice = device;
    
    if (textureDes)
    {
        mTexture = [device newTextureWithDescriptor:textureDes];
    }
}

MTLTextureBase::~MTLTextureBase()
{
}

void MTLTextureBase::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    if (mTexture == nil || pixelBytes == nullptr)
    {
        return;
    }
    
    MTLRegion region = MTLRegionMake3D(rect.offsetX, rect.offsetY, 0, rect.width, rect.height, 1);
    [mTexture replaceRegion:region mipmapLevel:level slice:slice
                  withBytes:pixelBytes bytesPerRow:bytesPerRow bytesPerImage:bytesPerImage];
}

void MTLTextureBase::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
}

uint32_t MTLTextureBase::GetWidth() const
{
    return (uint32_t)mTexture.width;
}

uint32_t MTLTextureBase::GetHeight() const
{
    return (uint32_t)mTexture.height;
}

uint32_t MTLTextureBase::GetDepth() const
{
    return (uint32_t)mTexture.depth;
}

#pragma mark MTLRCTexture2D

MTLRCTexture2D::MTLRCTexture2D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes)
{
}

MTLRCTexture2D::~MTLRCTexture2D()
{
    //
}

void MTLRCTexture2D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow)
{
    MTLTextureBase::ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
}

#pragma mark MTLRCTexture3D

MTLRCTexture3D::MTLRCTexture3D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes)
{
}

MTLRCTexture3D::~MTLRCTexture3D()
{
    //
}

void MTLRCTexture3D::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark MTLRCTextureCube

MTLRCTextureCube::MTLRCTextureCube(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes)
{
}

MTLRCTextureCube::~MTLRCTextureCube()
{
    //
}

void MTLRCTextureCube::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

#pragma mark MTLRCTexture2DArray

MTLRCTexture2DArray::MTLRCTexture2DArray(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes)
    : MTLTextureBase(device, commandQueue, textureDes)
{
}

MTLRCTexture2DArray::~MTLRCTexture2DArray()
{
    //
}

void MTLRCTexture2DArray::ReplaceRegion(const Rect2D& rect,
                    uint32_t level,
                    uint32_t slice,
                    const uint8_t* pixelBytes,
                    uint32_t bytesPerRow,
                    uint32_t bytesPerImage)
{
    MTLTextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

NAMESPACE_RENDERCORE_END
