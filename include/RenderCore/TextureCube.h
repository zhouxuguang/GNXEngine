//
//  TextureCube.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/6.
//

#ifndef RENDERCORE_TEXTURECUBE_INCLUDE_NHNDSVN
#define RENDERCORE_TEXTURECUBE_INCLUDE_NHNDSVN

#include "RenderDefine.h"
#include "RenderDescriptor.h"

NAMESPACE_RENDERCORE_BEGIN

class TextureCube
{
public:
    TextureCube(const std::vector<TextureDescriptor>& desArray);
    
    virtual ~TextureCube();
    
    /**
      set image data

     @param imageData image data
     */
    virtual void setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData) = 0;
    
    /**
     纹理是否有效

     @return ture or false
     */
    virtual bool isValid() const = 0;
};

typedef std::shared_ptr<TextureCube> TextureCubePtr;

NAMESPACE_RENDERCORE_END

#endif /* RENDERCORE_TEXTURECUBE_INCLUDE_NHNDSVN */
