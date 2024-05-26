//
//  VKTextureCube.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKTextureCube.h"

NAMESPACE_RENDERCORE_BEGIN

VKTextureCube::VKTextureCube(VulkanContextPtr context, const std::vector<TextureDescriptor>& desArray) : TextureCube(desArray)
{
    mContext = context;
}

VKTextureCube::~VKTextureCube()
{
    Release();
}

void VKTextureCube::setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData)
{
}

bool VKTextureCube::isValid() const
{
    return mImage != VK_NULL_HANDLE;
}

void VKTextureCube::Release()
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

    if (mVulkanImageViewPtr)
    {
        mVulkanImageViewPtr = nullptr;
    }
}

NAMESPACE_RENDERCORE_END
