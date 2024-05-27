//
//  VKTexture2D.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#ifndef GNX_ENGINE_VKTEXTURE2D_INCLUDE_JDSFHNFHGSDKNVSB
#define GNX_ENGINE_VKTEXTURE2D_INCLUDE_JDSFHNFHGSDKNVSB

#include "VulkanContext.h"
#include "Texture2D.h"

NAMESPACE_RENDERCORE_BEGIN

class VKTexture2D : public Texture2D
{
public:

    VKTexture2D(const VulkanContextPtr& context, const TextureDescriptor& des);

    ~VKTexture2D();
    
    virtual void allocMemory()
    {
    }

    virtual void setTextureData(const unsigned char* imageData);

    /**
      更新纹理数据

     @param rect 更新纹理区域
     @param mipmapLevel 纹理等级
     @param image_data 更新数据
     */
    virtual void replaceRegion(const Rect2D& rect, const unsigned char* imageData, unsigned int mipMapLevel = 0);
    
    virtual bool isValid() const
    {
        return mImage != VK_NULL_HANDLE;
    }

    const VulkanImageViewPtr getVKImageView() const noexcept {return mVulkanImageViewPtr;}
    
    //生成mipmap纹理
    void generateMipmapsTexture(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    
private:
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;

    VulkanImageViewPtr mVulkanImageViewPtr = nullptr;
    
    TextureDescriptor mTextureDes;

    void release();

    void createTexture(const VkDevice device, const TextureDescriptor& des);
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VKTEXTURE2D_INCLUDE_JDSFHNFHGSDKNVSB */
