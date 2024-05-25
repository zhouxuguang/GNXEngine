//
//  MTLTexture2D.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_TEXTURE_2D_INCLUDE_H
#define GNX_ENGINE_TEXTURE_2D_INCLUDE_H

#include "MTLRenderDefine.h"
#include "Texture2D.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLTexture2D : public Texture2D
{
public:
    MTLTexture2D(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const TextureDescriptor& des);
    
    ~MTLTexture2D();
    
public:
    
    void allocMemory() {}
    
    /**
      set image data

     @param imageData image data
     */
    virtual void setTextureData(const uint8_t* imageData);
    
    /**
      更新纹理数据
     
     @param rect 更新纹理区域
     @param mipMapLevel 纹理等级
     @param imageData 跟新数据
     */
    virtual void replaceRegion(const Rect2D& rect, const uint8_t* imageData, uint32_t mipMapLevel = 0);
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool isValid() const;
    
    id<MTLTexture> getMTLTexture(){return mTexture;};

private:
    id<MTLTexture> mTexture;
    id<MTLCommandQueue> mCommandQueue;
    id<MTLDevice> mDevice;
    
    NSUInteger mBytesPerRow = 0;
    
    void GenerateMipmapsTexture();
};

//创建默认的深度纹理和模板纹理
id<MTLTexture> createDepthStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

//创建深度纹理
id<MTLTexture> createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

//创建模板纹理
id<MTLTexture> createStencilTexture(id<MTLDevice> device, uint32_t width, uint32_t height);

NAMESPACE_RENDERCORE_END


#endif /* GNX_ENGINE_TEXTURE_2D_INCLUDE_H */
