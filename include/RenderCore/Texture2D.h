//
//  Texture2D.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_TEXTURE2D_INCLUDE_H
#define GNX_ENGINE_TEXTURE2D_INCLUDE_H

#include "RenderDefine.h"
#include "RenderDescriptor.h"

NAMESPACE_RENDERCORE_BEGIN


class Texture2D
{
public:
    Texture2D(const TextureDescriptor& des);
    
    virtual ~Texture2D();
    
    /**
     分配内存
     */
    virtual void allocMemory() = 0;
    
    /**
      set image data

     @param imageData image data
     */
    virtual void setTextureData(const uint8_t* imageData) = 0;
    
    /**
      更新纹理数据
     
     @param rect 更新纹理区域
     @param mipMapLevel 纹理等级
     @param imageData 跟新数据
     */
    virtual void replaceRegion(const Rect2D& rect, const uint8_t* imageData, uint32_t mipMapLevel = 0) = 0;
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool isValid() const = 0;
};

typedef std::shared_ptr<Texture2D> Texture2DPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_TEXTURE2D_INCLUDE_H */
