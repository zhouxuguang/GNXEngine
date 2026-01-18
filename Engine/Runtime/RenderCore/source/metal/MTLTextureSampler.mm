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
    
    // TODO : 这里还有问题需要解决，命名还需要规范化
//    enum SamplerMipFilter
//    {
//        MIN_NEAREST_MIPMAP_NEAREST = 0,
//        MIN_LINEAR_MIPMAP_NEAREST = 1,
//        MIN_NEAREST_MIPMAP_LINEAR = 2,
//        MIN_LINEAR_MIPMAP_LINEAR = 3
//    };
    switch (des.filterMip)
    {
        case SamplerMipFilter::MIN_NEAREST_MIPMAP_NEAREST:
        case SamplerMipFilter::MIN_NEAREST_MIPMAP_LINEAR:
            samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;
            break;
        case SamplerMipFilter::MIN_LINEAR_MIPMAP_LINEAR:
        case SamplerMipFilter::MIN_LINEAR_MIPMAP_NEAREST:
            samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
        default:
            break;
    }
    
    samplerDescriptor.rAddressMode = transToMTLAdressMode(des.wrapR);
    samplerDescriptor.tAddressMode = transToMTLAdressMode(des.wrapT);
    samplerDescriptor.sAddressMode = transToMTLAdressMode(des.wrapS);
    //samplerDescriptor.compareFunction = (MTLCompareFunction)des.compareFunc;
    return samplerDescriptor;
}

MTLSamplerAddressMode MTLTextureSampler::transToMTLAdressMode(SamplerWrapMode mode) noexcept
{
    MTLSamplerAddressMode adressMode;
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
