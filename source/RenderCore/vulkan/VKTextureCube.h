//
//  VKTextureCube.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_TEXTURE_CUBE_INCLUDEGNFDJS
#define GNX_ENGINE_VK_TEXTURE_CUBE_INCLUDEGNFDJS

#include "VulkanContext.h"
#include "TextureCube.h"

NAMESPACE_RENDERCORE_BEGIN

class VKTextureCube : public TextureCube
{
public:
    VKTextureCube(VulkanContextPtr context, const std::vector<TextureDescriptor>& desArray);
    
    ~VKTextureCube();
    
    virtual void setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData);
    
    virtual bool isValid() const;
    
    VulkanImageViewPtr GetImageView() const
    {
        return mVulkanImageViewPtr;
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    
    TextureDescriptor mTextureDesc;

    VulkanImageViewPtr mVulkanImageViewPtr = nullptr;
    
    void Release();
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_TEXTURE_CUBE_INCLUDEGNFDJS */
