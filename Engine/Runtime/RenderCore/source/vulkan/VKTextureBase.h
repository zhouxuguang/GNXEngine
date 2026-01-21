//
//  VKTextureBase.h
//  rendercore
//
//  Created by zhouxuguang on 2025/9/20.
//

#ifndef GNX_ENGINE_VKTEXTUREBASE_INCLUDE_JDSFHNDSGFG
#define GNX_ENGINE_VKTEXTUREBASE_INCLUDE_JDSFHNDSGFG

#include "VulkanContext.h"
#include "RCTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class VKTextureBase : virtual public RCTexture
{
public:
    VKTextureBase(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    virtual ~VKTextureBase();

    void CreateImageViews(const VkImageCreateInfo& imageCreateInfo);
    
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
    
    virtual bool IsValid() const;
    
    virtual uint32_t GetWidth() const;
    
    virtual uint32_t GetHeight() const;
    
    virtual uint32_t GetDepth() const;
    
    virtual uint32_t GetMipLevels() const;
    
    virtual uint32_t GetLayerCount() const;

    virtual void SetName(const char* name);
    
    VulkanImageViewPtr GetImageView() const
    {
        return mVulkanImageViewPtr;
    }

    VulkanImageViewPtr GetRenderTargetImageView(uint32_t targetSlice) const
    {
        return mRenderTargetViews[targetSlice];
    }
    
    VkFormat GetVKFormat() const
    {
        return mFormat;
    }
    
    VkImage GetVKImage() const
    {
        return mImage;
    }

    VkImageLayout GetCurrentLayout() const
    {
        return mCurrentLayout;
    }

    void SetCurrentLayout(VkImageLayout layout)
    {
        mCurrentLayout = layout;
    }

private:
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mDepth = 0;
    uint32_t mMipLevels = 1;
    uint32_t mLayerCount = 1;
    VulkanImageViewPtr mVulkanImageViewPtr = nullptr;
    std::vector<VulkanImageViewPtr> mRenderTargetViews;
    bool mSupportHostImageCopy = false;
    VkImageLayout mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

using VKTextureBasePtr = std::shared_ptr<VKTextureBase>;

#pragma mark VKRCTexture2D

class VKRCTexture2D : public VKTextureBase, public RCTexture2D
{
public:
    VKRCTexture2D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTexture2D();
    
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

using VKRCTexture2DPtr = std::shared_ptr<VKRCTexture2D>;

#pragma mark VKRCTexture3D

class VKRCTexture3D : public VKTextureBase, public RCTexture3D
{
public:
    VKRCTexture3D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTexture3D();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
};

using VKRCTexture3DPtr = std::shared_ptr<VKRCTexture3D>;

#pragma mark VKRCTextureCube

class VKRCTextureCube : public VKTextureBase, public RCTextureCube
{
public:
    VKRCTextureCube(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTextureCube();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
};

using VKRCTextureCubePtr = std::shared_ptr<VKRCTextureCube>;

#pragma mark VKRCTexture2DArray

class VKRCTexture2DArray : public VKTextureBase, public RCTexture2DArray
{
public:
    VKRCTexture2DArray(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTexture2DArray();
    
    virtual void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage);
};

using VKRCTexture2DArrayPtr = std::shared_ptr<VKRCTexture2DArray>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKTEXTUREBASE_INCLUDE_JDSFHNDSGFG */
