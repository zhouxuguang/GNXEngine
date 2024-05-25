//
//  VKDepthStencilBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNXENGINE_VK_DEPTHSCTENCIL_BUFFER_INCLUDE_DJSGJJG
#define GNXENGINE_VK_DEPTHSCTENCIL_BUFFER_INCLUDE_DJSGJJG

#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

//默认的深度模板缓冲纹理
class VKDepthStencilBuffer
{
public:
    VKDepthStencilBuffer(VulkanContextPtr context, uint32_t width, uint32_t height);

    ~VKDepthStencilBuffer();

    void CreateDepthStencilBuffer(VulkanContextPtr context, uint32_t width, uint32_t height);
    
    VkFormat GetFormat() const;

    VkImage GetImage() const;
    
    VkImageView GetImageView() const;
    
    void Release();

private:
    VkImage mDepthStencilImage = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VkImageView mDepthStencilImageView = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
    VkFormat mFormat;
};

using VKDepthStencilBufferPtr = std::shared_ptr<VKDepthStencilBuffer>;

// 深度模板测试的描述
class VKDepthStencilState
{
public:
    VKDepthStencilState(const DepthStencilDescriptor& des);
    
    ~VKDepthStencilState();
    
    const VkPipelineDepthStencilStateCreateInfo& GetDepthStencilStateCreateInfo() const
    {
        return mDepthStencilCreateInfo;
    }
    
private:
    VkPipelineDepthStencilStateCreateInfo mDepthStencilCreateInfo;
};

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_VK_DEPTHSCTENCIL_BUFFER_INCLUDE_DJSGJJG */
