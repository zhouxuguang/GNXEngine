//
//  RCTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ
#define GNX_ENGINE_RCTEXTURE_INCLUDE_FHJDSVJ

#include "RenderDefine.h"
#include "TextureFormat.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief RHI纹理的基类
 * 
 */
class RCTexture
{
public:
    RCTexture(TextureType textureType)
    {
        mTextureType = textureType;
    }
    
    virtual ~RCTexture(){}
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool IsValid() const = 0;
    
    virtual uint32_t GetWidth() const = 0;
    
    virtual uint32_t GetHeight() const = 0;
    
    virtual uint32_t GetDepth() const = 0;
    
    virtual uint32_t GetMipLevels() const = 0;
    
    virtual uint32_t GetLayerCount() const = 0;

    virtual void SetName(const char* name) = 0;
    
    TextureType GetTextureType() const
    {
        return mTextureType;
    }
    
    TextureFormat GetTextureFormat() const
    {
        return mFormat;
    }
    
    void SetFormat(TextureFormat format)
    {
        if (!mInited)
        {
            mFormat = format;
            mInited = true;
        }
    }
    
private:
    TextureType mTextureType = TextureType_Unkown;
    TextureFormat mFormat = kTexFormatInvalid;
    bool mInited = false;
};

typedef std::shared_ptr<RCTexture> RCTexturePtr;

enum class TextureUploadStatus { Pending, Complete, Failed };

class TextureUpload
{
public:
    virtual ~TextureUpload() = default;
    virtual TextureUploadStatus GetStatus() const = 0;
};
using TextureUploadPtr = std::shared_ptr<TextureUpload>;

class CompletedTextureUpload final : public TextureUpload
{
public:
    TextureUploadStatus GetStatus() const override { return TextureUploadStatus::Complete; }
};

class FailedTextureUpload final : public TextureUpload
{
public:
    TextureUploadStatus GetStatus() const override { return TextureUploadStatus::Failed; }
};

/**
 * @brief RHI 2D纹理
 * 
 */
class RCTexture2D : virtual public RCTexture
{
public:
    RCTexture2D() : RCTexture(TextureType_2D){}
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

    /**
       同步更新纹理数据：函数返回时数据已经上传完成，纹理处于可采样状态。

       异步实现（Vulkan 默认的 ReplaceRegion 会提交到传输队列且不与图形队列同步）
       只保证 CPU 侧提交，GPU 侧可能还没做 layout 转换，紧接着采样该纹理属于未定义
       行为（读取到 UNDEFINED 布局的图像，NVIDIA 等驱动会直接报
       VK_ERROR_DEVICE_LOST）。「上传后立即使用」的场景（如流式加载的瓦片纹理）
       必须使用本接口。

       默认实现退化为 ReplaceRegion（D3D12/Metal 后端的 ReplaceRegion 本身同步）。
     */
    virtual void ReplaceRegionSync(const Rect2D& rect,
                        uint32_t level,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow)
    {
        ReplaceRegion(rect, level, pixelBytes, bytesPerRow);
    }

    // The caller owns the upload ticket and must not sample this texture until
    // it reports Complete. Backends without a transfer path may upload inline.
    virtual TextureUploadPtr ReplaceRegionAsync(const Rect2D& rect,
                        uint32_t level,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow)
    {
        ReplaceRegionSync(rect, level, pixelBytes, bytesPerRow);
        return std::make_shared<CompletedTextureUpload>();
    }
};

typedef std::shared_ptr<RCTexture2D> RCTexture2DPtr;

/**
 * @brief RHI 3D纹理
 * 
 */
class RCTexture3D : virtual public RCTexture
{
public:
    RCTexture3D() : RCTexture(TextureType_3D){}
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
class RCTextureCube : virtual public RCTexture
{
public:
    RCTextureCube() : RCTexture(TextureType_CUBE){}
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
class RCTexture2DArray : virtual public RCTexture
{
public:
    RCTexture2DArray() : RCTexture(TextureType_2D_ARRAY){}
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
