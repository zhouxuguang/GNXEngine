//
//  VKTextureSampler.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#ifndef GNXENGINE_VK_TEXTURE_SAMPLER_INCLUDE_HSDGHJDGJH
#define GNXENGINE_VK_TEXTURE_SAMPLER_INCLUDE_HSDGHJDGJH

#include "VulkanContext.h"
#include "TextureSampler.h"

NAMESPACE_RENDERCORE_BEGIN

class VKTextureSampler : public TextureSampler
{
public:
    VKTextureSampler(VulkanContextPtr context, const SamplerDescriptor& des);

    ~VKTextureSampler();

    const VkSampler GetVKSampler() const { return mSampler; }
    
private:
    void GenerateSamplerInfo(const SamplerDescriptor& des, VkSamplerCreateInfo& samplerInfo);

    VkSamplerAddressMode TransToVulkanAdressMode(SamplerWrapMode mode);

private:
    VkSampler mSampler = VK_NULL_HANDLE;
    VulkanContextPtr mContext = nullptr;
};

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_VK_TEXTURE_SAMPLER_INCLUDE_HSDGHJDGJH */
