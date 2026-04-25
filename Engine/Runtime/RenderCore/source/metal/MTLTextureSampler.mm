//
//  MTLTextureSampler.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLTextureSampler.h"

NAMESPACE_RENDERCORE_BEGIN

MTLTextureSampler::~MTLTextureSampler()
{
    mSamplerState = nil;
}

MTLTextureSampler::MTLTextureSampler(id<MTLDevice> device, const SamplerDesc& des) noexcept : TextureSampler(des)
{
    MTLSamplerDescriptor* samplerDescriptor = transToMTLSamplerDescriptor(des);
    mSamplerState = [device newSamplerStateWithDescriptor:samplerDescriptor];
}

MTLSamplerDescriptor* MTLTextureSampler::transToMTLSamplerDescriptor(const SamplerDesc& des) noexcept
{
    MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];
    switch (des.filterMin)
    {
        case SamplerMinFilter::MIN_NEAREST:
            samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
            samplerDescriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
            break;
        case SamplerMinFilter::MIN_LINEAR:
            samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
            break;
        default:
            break;
    }
    
    switch (des.filterMag)
    {
        case SamplerMagFilter::MAG_NEAREST:
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
            break;
        case SamplerMagFilter::MAG_LINEAR:
            samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
            break;
        default:
            break;
    }
    
    switch (des.filterMip)
    {
        case SamplerMipFilter::MIN_NEAREST_MIPMAP_NEAREST:
        case SamplerMipFilter::MIN_NEAREST_MIPMAP_LINEAR:
            samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;
            break;
        case SamplerMipFilter::MIN_LINEAR_MIPMAP_LINEAR:
        case SamplerMipFilter::MIN_LINEAR_MIPMAP_NEAREST:
            samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
            break;
        default:
            break;
    }
    
    samplerDescriptor.rAddressMode = transToMTLAdressMode(des.wrapR);
    samplerDescriptor.tAddressMode = transToMTLAdressMode(des.wrapT);
    samplerDescriptor.sAddressMode = transToMTLAdressMode(des.wrapS);
    
    // 深度比较函数（阴影映射、PCF 过滤等）
    if (des.compareMode == SamplerCompareMode::COMPARE_TO_TEXTURE)
    {
        samplerDescriptor.compareFunction = (MTLCompareFunction)des.compareFunc;
    }
    
    // 各向异性过滤
    if (des.anisotropyLog2 > 0)
    {
        samplerDescriptor.maxAnisotropy = 1 << des.anisotropyLog2;
    }
    
    // LOD 钳制
    samplerDescriptor.lodMinClamp = (float)des.minLod;
    samplerDescriptor.lodMaxClamp = (float)des.maxLod;
    
    return samplerDescriptor;
}

MTLSamplerAddressMode MTLTextureSampler::transToMTLAdressMode(SamplerWrapMode mode) noexcept
{
    MTLSamplerAddressMode adressMode = MTLSamplerAddressModeClampToEdge;
    switch (mode)
    {
        case SamplerWrapMode::CLAMP_TO_EDGE:
            adressMode = MTLSamplerAddressModeClampToEdge;
            break;
        case SamplerWrapMode::REPEAT:
            adressMode = MTLSamplerAddressModeRepeat;
            break;
        case SamplerWrapMode::MIRRORED_REPEAT:
            adressMode = MTLSamplerAddressModeMirrorRepeat;
            break;
        default:
            break;
    }
    return adressMode;
}

NAMESPACE_RENDERCORE_END
