//
//  GLRenderTexture.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#include "GLRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

GLRenderTexture::GLRenderTexture(const TextureDescriptor& des) : RenderTexture(des)
{
    mTexture = 0;
    
    if (0 == des.width || 0 == des.height)
    {
        assert(false);
        return;
    }
    
    if (mTexture == 0)
    {
        glGenTextures(1, &mTexture);
    }
    
    if (mTexture == 0)
    {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, mTexture);
    
    //这里认为创建成功
    mTextureDes = des;
    mGLESFormat = GetTransferFormatGLES(des.format);
    
    GLuint width = mTextureDes.width;
    GLuint height = mTextureDes.height;
    
    glTexStorage2D(GL_TEXTURE_2D, 1, mGLESFormat.internalFormat, width, height);
}

GLRenderTexture::~GLRenderTexture()
{
    if (mTexture)
    {
        glDeleteTextures(1, &mTexture);
        mTexture = 0;
    }
}

uint32_t GLRenderTexture::getWidth() const
{
    if (mTexture)
    {
        return mTextureDes.width;
    }
    return 0;
}

uint32_t GLRenderTexture::getHeight() const
{
    if (mTexture)
    {
        return mTextureDes.height;
    }
    return 0;
}

TextureFormat GLRenderTexture::getTextureFormat() const
{
    if (mTexture)
    {
        return mTextureDes.format;
    }
    return 0;
}

void GLRenderTexture::apply(GLuint nTextureUint)
{
    if (mTexture)
    {
        GLenum textUnit = GL_TEXTURE0 + nTextureUint;
        glActiveTexture(textUnit);
        
        glBindTexture(GL_TEXTURE_2D, mTexture);
    }
}

NAMESPACE_RENDERCORE_END
