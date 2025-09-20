//
//  RCTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ
#define GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief RHI纹理的基类
 * 
 */
class RCTexture
{
public:
    RCTexture(){}
    
    virtual ~RCTexture(){}
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool IsValid() const = 0;
};

typedef std::shared_ptr<RCTexture> RCTexturePtr;

/**
 * @brief RHI 2D纹理
 * 
 */
class RCTexture2D : public RCTexture
{
public:
    RCTexture2D(){}
    virtual ~RCTexture2D(){}

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
                        uint32_t bytesPerRow) = 0;
};

typedef std::shared_ptr<RCTexture2D> RCTexture2DPtr;

/**
 * @brief RHI 3D纹理
 * 
 */
class RCTexture3D : public RCTexture
{
public:
    RCTexture3D(){}
    virtual ~RCTexture3D(){}
    
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
                        uint32_t bytesPerImage) = 0;
};

using RCTexture3DPtr = std::shared_ptr<RCTexture3D>;

/**
 * @brief RHI cube纹理
 * 
 */
class RCTextureCube : public RCTexture
{
public:
    RCTextureCube(){}
    virtual ~RCTextureCube(){}
    
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
                        uint32_t bytesPerImage) = 0;
};

using RCTextureCubePtr = std::shared_ptr<RCTextureCube>;

/**
 * @brief RHI 2d array纹理
 *
 */
class RCTexture2DArray : public RCTexture
{
public:
    RCTexture2DArray(){}
    virtual ~RCTexture2DArray(){}
    
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
                        uint32_t bytesPerImage) = 0;
};

using RCTexture2DArrayPtr = std::shared_ptr<RCTexture2DArray>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ */
