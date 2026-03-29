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
#include <unordered_map>

NAMESPACE_RENDERCORE_BEGIN

class MTLTextureBase : virtual public RCTexture
{
public:
    MTLTextureBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, MTLTextureDescriptor *textureDes);
    virtual ~MTLTextureBase();
    
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
    
    virtual uint32_t GetMipLevels() const;
    
    virtual uint32_t GetLayerCount() const;

    virtual void SetName(const char* name);
    
    id<MTLTexture> getMTLTexture() {return mTexture;};

    /**
     获取指定 mipLevel + slice 的 texture view，自动缓存
     mipLevel=0 且 slice=0 时直接返回原始纹理，不创建 view
     */
    id<MTLTexture> getMTLTextureView(uint32_t mipLevel, uint32_t slice);
    
private:
    id<MTLTexture> mTexture;
    id<MTLCommandQueue> mCommandQueue;
    id<MTLDevice> mDevice;

    /// texture view 的缓存 key
    struct TextureViewKey
    {
        uint32_t mipLevel;
        uint32_t slice;

        bool operator==(const TextureViewKey& other) const
        {
            return mipLevel == other.mipLevel && slice == other.slice;
        }
    };

    /// TextureViewKey 的 hash functor
    struct TextureViewKeyHash
    {
        std::size_t operator()(const TextureViewKey& key) const
        {
            std::size_t h = std::hash<uint32_t>()(key.mipLevel);
            h ^= std::hash<uint32_t>()(key.slice) << 1;
            return h;
        }
    };

    std::unordered_map<TextureViewKey, id<MTLTexture>, TextureViewKeyHash> mTextureViews;
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
};

using MTLRCTexture2DArrayPtr = std::shared_ptr<MTLRCTexture2DArray>;

//创建默认的深度纹理和模板纹理
id<MTLTexture> createDepthStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

//创建深度纹理
id<MTLTexture> createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

//创建模板纹理
id<MTLTexture> createStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTLTEXTURE_BASE_INCLUDE_H_HDSJJD */
