//
//  MTLTextureBase.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTLTEXTURE_BASE_INCLUDE_H_HDSJJD
#define GNX_ENGINE_MTLTEXTURE_BASE_INCLUDE_H_HDSJJD

#include "MTLRenderDefine.h"
#include "RCTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLTextureBase : public RCTexture
{
public:
    MTLTextureBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    ~MTLTextureBase();
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param slice 切片索引
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     @param bytesPerImage 每个切片的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow);
    
    virtual bool IsValid() const
    {
        return mTexture != nil;
    }
    
    virtual uint32_t GetWidth() const;
    
    virtual uint32_t GetHeight() const;
    
    virtual uint32_t GetDepth() const;
    
    id<MTLTexture> getMTLTexture() {return mTexture;};
    
private:
    id<MTLTexture> mTexture;
    id<MTLCommandQueue> mCommandQueue;
    id<MTLDevice> mDevice;
};

using MTLTextureBasePtr = std::shared_ptr<MTLTextureBase>;

#pragma mark MTLRCTexture2D

class MTLRCTexture2D : public MTLTextureBase, public RCTexture2D
{
public:
    MTLRCTexture2D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    
    ~MTLRCTexture2D();
    
    /**
       更新纹理数据
     
     @param rect 更新纹理区域
     @param level 纹理mipmap等级
     @param pixelBytes 纹理数据
     @param bytesPerRow 每行的字节数
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow);
    
    virtual bool IsValid() const
    {
        return MTLTextureBase::IsValid();
    }
    
    virtual uint32_t GetWidth() const
    {
        return MTLTextureBase::GetWidth();
    }
    
    virtual uint32_t GetHeight() const
    {
        return MTLTextureBase::GetHeight();
    }
    
    virtual uint32_t GetDepth() const
    {
        return MTLTextureBase::GetDepth();
    }
};

using MTLRCTexture2DPtr = std::shared_ptr<MTLRCTexture2D>;

#pragma mark MTLRCTexture3D

class MTLRCTexture3D : public MTLTextureBase, public RCTexture3D
{
public:
    MTLRCTexture3D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    
    ~MTLRCTexture3D();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
    
    virtual bool IsValid() const
    {
        return MTLTextureBase::IsValid();
    }
    
    virtual uint32_t GetWidth() const
    {
        return MTLTextureBase::GetWidth();
    }
    
    virtual uint32_t GetHeight() const
    {
        return MTLTextureBase::GetHeight();
    }
    
    virtual uint32_t GetDepth() const
    {
        return MTLTextureBase::GetDepth();
    }
};

using MTLRCTexture3DPtr = std::shared_ptr<MTLRCTexture3D>;

#pragma mark MTLRCTextureCube

class MTLRCTextureCube : public MTLTextureBase, public RCTextureCube
{
public:
    MTLRCTextureCube(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    
    ~MTLRCTextureCube();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
    
    virtual bool IsValid() const
    {
        return MTLTextureBase::IsValid();
    }
    
    virtual uint32_t GetWidth() const
    {
        return MTLTextureBase::GetWidth();
    }
    
    virtual uint32_t GetHeight() const
    {
        return MTLTextureBase::GetHeight();
    }
    
    virtual uint32_t GetDepth() const
    {
        return MTLTextureBase::GetDepth();
    }
};

using MTLRCTextureCubePtr = std::shared_ptr<MTLRCTextureCube>;

#pragma mark MTLRCTexture2DArray

class MTLRCTexture2DArray : public MTLTextureBase, public RCTexture2DArray
{
public:
    MTLRCTexture2DArray(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    
    ~MTLRCTexture2DArray();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
    
    virtual bool IsValid() const
    {
        return MTLTextureBase::IsValid();
    }
    
    virtual uint32_t GetWidth() const
    {
        return MTLTextureBase::GetWidth();
    }
    
    virtual uint32_t GetHeight() const
    {
        return MTLTextureBase::GetHeight();
    }
    
    virtual uint32_t GetDepth() const
    {
        return MTLTextureBase::GetDepth();
    }
};

using MTLRCTexture2DArrayPtr = std::shared_ptr<MTLRCTexture2DArray>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTLTEXTURE_BASE_INCLUDE_H_HDSJJD */
