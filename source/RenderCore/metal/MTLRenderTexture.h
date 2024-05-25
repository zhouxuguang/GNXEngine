//
//  MTLRenderTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#ifndef GNX_ENGINE_MTL_RENDER_TEXTURE_INCLUDE_HLFKJK
#define GNX_ENGINE_MTL_RENDER_TEXTURE_INCLUDE_HLFKJK

#include "MTLRenderDefine.h"
#include "RenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLRenderTexture : public RenderTexture
{
public:
    MTLRenderTexture(id<MTLDevice> device, const TextureDescriptor& des);
    
    ~MTLRenderTexture();
    
    virtual uint32_t getWidth() const;
    
    virtual uint32_t getHeight() const;
    
    virtual TextureFormat getTextureFormat() const;
    
    id<MTLTexture> getMTLTexture() const
    {
        return mTexture;
    }
    
private:
    id<MTLTexture> mTexture = nil;
    TextureFormat mTextureFormat = kTexFormatInvalid;
};

typedef std::shared_ptr<MTLRenderTexture> MTLRenderTexturePtr;

NAMESPACE_RENDERCORE_END


#endif /* GNX_ENGINE_MTL_RENDER_TEXTURE_INCLUDE_HLFKJK */
