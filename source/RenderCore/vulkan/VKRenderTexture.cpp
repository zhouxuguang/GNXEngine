//
//  VKRenderTexture.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKRenderTexture.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKRenderTexture::VKRenderTexture(VulkanContextPtr context, const TextureDescriptor& des) : RenderTexture(des), mTextureDes(des)
{
    mContext = context;
    
    VkFormat format = VulkanBufferUtil::ConvertTextureFormat(des.format);
    assert(format != VK_FORMAT_UNDEFINED);
    mFormat = format;
    assert(des.width > 0 && des.height > 0);

    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    VulkanBufferUtil::CreateImage2DVMA(mContext->vmaAllocator, des.width, des.height, format,
                                       VK_SAMPLE_COUNT_1_BIT, 1, VK_IMAGE_TILING_OPTIMAL, imageUsageFlags, mImage, mAllocation);
    
    //创建图像视图
    
    // | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT 这两个标记加上后图像就是红色的，也不知道为啥，
    // 需要继续研究，应该需要判断缺失是深度模板的格式后再加上这些标记
    VkImageView imageView = VulkanBufferUtil::CreateImageView(mContext->device, mImage, format, nullptr,
                            VK_IMAGE_ASPECT_COLOR_BIT, 1);
    mVulkanImageViewPtr = std::make_shared<VulkanImageView>(mContext->device, imageView);
}

VKRenderTexture::~VKRenderTexture()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }

    if (mImage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(mContext->vmaAllocator, mImage, mAllocation);
        mImage = VK_NULL_HANDLE;
    }
    
    if (mAllocation)
    {
        vmaFreeMemory(mContext->vmaAllocator, mAllocation);
        mAllocation = VK_NULL_HANDLE;
    }

    if (mVulkanImageViewPtr)
    {
        mVulkanImageViewPtr = nullptr;
    }
}

uint32_t VKRenderTexture::getWidth() const
{
    return mTextureDes.width;
}

uint32_t VKRenderTexture::getHeight() const
{
    return mTextureDes.height;
}

TextureFormat VKRenderTexture::getTextureFormat() const
{
    return mTextureDes.format;
}

NAMESPACE_RENDERCORE_END
