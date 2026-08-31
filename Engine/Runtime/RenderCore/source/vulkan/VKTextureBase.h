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
    
    bool IsValid() const override;
    
    uint32_t GetWidth() const override;
    
    uint32_t GetHeight() const override;
    
    uint32_t GetDepth() const override;
    
    uint32_t GetMipLevels() const override;
    
    uint32_t GetLayerCount() const override;

    void SetName(const char* name) override;
    
    VulkanImageViewPtr GetImageView() const
    {
        return mVulkanImageViewPtr;
    }

    VulkanImageViewPtr GetRenderTargetImageView(uint32_t targetSlice) const
    {
        return mRenderTargetViews[targetSlice];
    }
    
    /**
     获取特定 mip level 的 ImageView（用于 compute shader 写入特定 level）
     
     @param mipLevel mip level 索引
     @return 指向该 mip level 的 ImageView
     */
    VulkanImageViewPtr GetMipLevelImageView(uint32_t mipLevel);
    
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

    /**
     构建 ASTC LDR 解码模式扩展结构（VK_EXT_astc_decode_mode）。
     仅当扩展已启用且纹理格式为 ASTC LDR（UNORM/SRGB block）时返回 true，
     并填充 astcDecodeMode 结构供 VkImageViewCreateInfo::pNext 使用。
     */
    bool GetASTCDecodeMode(VkImageViewASTCDecodeModeEXT& astcDecodeMode) const;

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
    std::unordered_map<uint32_t, VulkanImageViewPtr> mMipLevelViews;  // 缓存每个 mip level 的视图
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
                        uint32_t bytesPerRow) override;
};

using VKRCTexture2DPtr = std::shared_ptr<VKRCTexture2D>;

#pragma mark VKRCTexture3D

class VKRCTexture3D : public VKTextureBase, public RCTexture3D
{
public:
    VKRCTexture3D(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTexture3D();
    
    void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) override;
};

using VKRCTexture3DPtr = std::shared_ptr<VKRCTexture3D>;

#pragma mark VKRCTextureCube

class VKRCTextureCube : public VKTextureBase, public RCTextureCube
{
public:
    VKRCTextureCube(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTextureCube();
    
    void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) override;
};

using VKRCTextureCubePtr = std::shared_ptr<VKRCTextureCube>;

#pragma mark VKRCTexture2DArray

class VKRCTexture2DArray : public VKTextureBase, public RCTexture2DArray
{
public:
    VKRCTexture2DArray(const VulkanContextPtr& context, const VkImageCreateInfo& imageCreateInfo);
    
    ~VKRCTexture2DArray();
    
    void ReplaceRegion(const Rect2D& rect,
                        uint32_t level,
                        uint32_t slice,
                        const uint8_t* pixelBytes,
                        uint32_t bytesPerRow,
                        uint32_t bytesPerImage) override;
};

using VKRCTexture2DArrayPtr = std::shared_ptr<VKRCTexture2DArray>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKTEXTUREBASE_INCLUDE_JDSFHNDSGFG */
