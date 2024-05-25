//
//  VKTextureSampler.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKTextureSampler.h"

NAMESPACE_RENDERCORE_BEGIN

VKTextureSampler::VKTextureSampler(VulkanContextPtr context, const SamplerDescriptor& des) :TextureSampler(des)
{
    mContext = context;
    VkSamplerCreateInfo samplerInfo;
    GenerateSamplerInfo(des, samplerInfo);

    if (vkCreateSampler(mContext->device, &samplerInfo, NULL, &mSampler) != VK_SUCCESS)
    {
        mSampler = VK_NULL_HANDLE;
        return;
    }
}

VKTextureSampler::~VKTextureSampler()
{
    if (mSampler)
    {
        vkDestroySampler(mContext->device, mSampler, nullptr);
        mSampler = VK_NULL_HANDLE;
    }
}

void VKTextureSampler::GenerateSamplerInfo(const SamplerDescriptor &des, VkSamplerCreateInfo &samplerInfo)
{
    switch (des.filterMag)
    {
        case SamplerMagFilter::MAG_LINEAR:
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            break;
        case SamplerMagFilter::MAG_NEAREST:
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            break;
        default: //should not go here
            break;
    }

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;

    switch (des.filterMin)
    {
        case SamplerMinFilter::MIN_LINEAR:
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            break;
        case SamplerMinFilter::MIN_NEAREST:
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            break;
        case SamplerMinFilter::MIN_LINEAR_MIPMAP_LINEAR:
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case SamplerMinFilter::MIN_LINEAR_MIPMAP_NEAREST:
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case SamplerMinFilter::MIN_NEAREST_MIPMAP_LINEAR:
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case SamplerMinFilter::MIN_NEAREST_MIPMAP_NEAREST:
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        default:
            break;
    }

    samplerInfo.addressModeU = TransToVulkanAdressMode(des.wrapS);
    samplerInfo.addressModeV = TransToVulkanAdressMode(des.wrapT);
    samplerInfo.addressModeW = TransToVulkanAdressMode(des.wrapR);

    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.flags = 0;

    //归一化纹理坐标
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    //各项异性
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = float(des.minLod);
    samplerInfo.maxLod = float(des.maxLod);

}

VkSamplerAddressMode VKTextureSampler::TransToVulkanAdressMode(SamplerWrapMode mode)
{
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    switch (mode)
    {
        case SamplerWrapMode::CLAMP_TO_EDGE:
            addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case SamplerWrapMode::REPEAT:
            addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case SamplerWrapMode::MIRRORED_REPEAT:
            addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        default:
            break;
    }
    return addressMode;
}

NAMESPACE_RENDERCORE_END
