//
//  VKRenderTexture.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_RENDER_TEXTURE_INCLUDE_KFJSDH
#define GNX_ENGINE_VK_RENDER_TEXTURE_INCLUDE_KFJSDH

#include "VulkanContext.h"
#include "RenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderTexture : public RenderTexture
{
public:
    VKRenderTexture(VulkanContextPtr context, const TextureDescriptor& des);
    
    ~VKRenderTexture();
    
    virtual uint32_t getWidth() const;
    
    virtual uint32_t getHeight() const;
    
    virtual TextureFormat getTextureFormat() const;
    
    VulkanImageViewPtr GetImageView() const
    {
        return mVulkanImageViewPtr;
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    
    VkImage mImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    VulkanImageViewPtr mVulkanImageViewPtr = nullptr;
    
    TextureDescriptor mTextureDes;
};

using VKRenderTexturePtr = std::shared_ptr<VKRenderTexture>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_TEXTURE_INCLUDE_KFJSDH */
