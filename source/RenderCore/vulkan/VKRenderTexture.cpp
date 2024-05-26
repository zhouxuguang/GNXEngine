//
//  VKRenderTexture.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

VKRenderTexture::VKRenderTexture(VulkanContextPtr context, const TextureDescriptor& des) : RenderTexture(des), mTextureDes(des)
{
    mContext = context;
}

VKRenderTexture::~VKRenderTexture()
{
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
