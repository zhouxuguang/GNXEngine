//
//  MTLTextureCube.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#ifndef GNX_ENGINE_MTL_TEXTURECUBE_INCLUDE_FGK
#define GNX_ENGINE_MTL_TEXTURECUBE_INCLUDE_FGK

#include "MTLRenderDefine.h"
#include "TextureCube.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLTextureCube : public TextureCube
{
public:
    MTLTextureCube(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const std::vector<TextureDescriptor>& desArray);
    
    ~MTLTextureCube();
    
    /**
      set image data

     @param imageData image data
     */
    virtual void setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData);
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool isValid() const;
    
    id<MTLTexture> getMTLTexture() const
    {
        return mTexture;
    }
    
private:
    id<MTLTexture> mTexture;
    id<MTLCommandQueue> mCommandQueue;
    id<MTLDevice> mDevice;
    
    MTLTextureDescriptor *mTextureDes;
    
    NSUInteger mBytesPerRow = 0;
};

typedef std::shared_ptr<MTLTextureCube> MTLTextureCubePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_TEXTURECUBE_INCLUDE_FGK */
