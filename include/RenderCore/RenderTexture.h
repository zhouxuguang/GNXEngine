//
//  RenderTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/15.
//

#ifndef GNXENGINE_RENDERTEXTURE_INCLUDE_HHH_
#define GNXENGINE_RENDERTEXTURE_INCLUDE_HHH_

#include "RenderDefine.h"
#include "TextureFormat.h"
#include "RenderDescriptor.h"

NAMESPACE_RENDERCORE_BEGIN

class RenderTexture
{
public:
    RenderTexture(const TextureDescriptor& des);
    
    virtual ~RenderTexture();
    
    virtual uint32_t getWidth() const = 0;
    
    virtual uint32_t getHeight() const = 0;
    
    virtual TextureFormat getTextureFormat() const = 0;
};

typedef std::shared_ptr<RenderTexture> RenderTexturePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_RENDERTEXTURE_INCLUDE_HHH_ */
