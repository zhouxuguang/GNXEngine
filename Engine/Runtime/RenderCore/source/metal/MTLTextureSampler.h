//
//  MTLTextureSampler.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_TEXTURESAMPLER_INCLUDE_H
#define GNX_ENGINE_MTL_TEXTURESAMPLER_INCLUDE_H

#include "MTLRenderDefine.h"
#include "TextureSampler.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLTextureSampler : public TextureSampler
{
public:
    MTLTextureSampler();
    
    ~MTLTextureSampler();
    
    MTLTextureSampler(id<MTLDevice> device, const SamplerDescriptor& des) noexcept;
    
    id<MTLSamplerState> getMTLSampler() noexcept {return mSamplerState;};
    
private:
    MTLSamplerDescriptor* transToMTLSamplerDescriptor(const SamplerDescriptor& des) noexcept;
    
    MTLSamplerAddressMode transToMTLAdressMode(SamplerWrapMode mode) noexcept;
    
private:
    id<MTLSamplerState> mSamplerState;
};

typedef std::shared_ptr<MTLTextureSampler> MTLTextureSamplerPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_TEXTURESAMPLER_INCLUDE_H */
