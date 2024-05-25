//
//  GLRenderTexture.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#ifndef GNX_ENGINE_GL_RENDER_TEXTURE_INCLUDE_KJDGJN_H
#define GNX_ENGINE_GL_RENDER_TEXTURE_INCLUDE_KJDGJN_H

#include "GLRenderDefine.h"
#include "RenderTexture.h"
#include "GLES3Untily.h"

NAMESPACE_RENDERCORE_BEGIN

class GLRenderTexture : public RenderTexture
{
public:
    GLRenderTexture(const TextureDescriptor& des);
    
    virtual ~GLRenderTexture();
    
    virtual uint32_t getWidth() const;
    
    virtual uint32_t getHeight() const;
    
    virtual TextureFormat getTextureFormat() const;
    
    void apply(GLuint nTextureUint);
    
    GLuint getTextureID() const
    {
        return mTexture;
    }
    
private:
    GLuint mTexture = 0;
    TextureDescriptor mTextureDes;
    TransferFormatGLES mGLESFormat;
};

typedef std::shared_ptr<GLRenderTexture> GLRenderTexturePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_RENDER_TEXTURE_INCLUDE_KJDGJN_H */
