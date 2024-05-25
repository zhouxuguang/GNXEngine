//
//  MTLRenderTexture.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#include "MTLRenderTexture.h"
#include "BitHacks.h"

NAMESPACE_RENDERCORE_BEGIN

MTLRenderTexture::MTLRenderTexture(id<MTLDevice> device, const TextureDescriptor& des) : RenderTexture(des)
{
    MTLPixelFormat format = ConvertTextureFormatToMetal(des.format);
    if (format == MTLPixelFormatInvalid)
    {
        assert(false);
        return;
    }
    
    if (0 == des.width || 0 == des.height)
    {
        assert(false);
        return;
    }
    
    //需要检查宽高，创建mipmap纹理时
    BOOL mipmap = (des.mipmaped && mathutil::IsPowerOfTwo(des.width) && mathutil::IsPowerOfTwo(des.height));
    
    MTLTextureDescriptor *textureDes = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:des.width height:des.height mipmapped:mipmap];
    if (textureDes)
    {
        if (@available(iOS 9.0, *))
        {
            // 属性设置为shader可读写以及rendertarget
            textureDes.resourceOptions = MTLResourceStorageModePrivate;
            textureDes.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
            textureDes.storageMode = MTLStorageModePrivate;
            if (@available(iOS 10.0, *))
            {
                //textureDes.storageMode = MTLStorageModeMemoryless;
            }
        }
        textureDes.textureType = MTLTextureType2D;
        
        mTexture = [device newTextureWithDescriptor:textureDes];
        mTextureFormat = des.format;
    }
}

MTLRenderTexture::~MTLRenderTexture()
{
    //
}

uint32_t MTLRenderTexture::getWidth() const
{
    if (mTexture)
    {
        return (uint32_t)mTexture.width;
    }
    return 0;
}

uint32_t MTLRenderTexture::getHeight() const
{
    if (mTexture)
    {
        return (uint32_t)mTexture.height;
    }
    return 0;
}

TextureFormat MTLRenderTexture::getTextureFormat() const
{
    return mTextureFormat;
}

NAMESPACE_RENDERCORE_END

