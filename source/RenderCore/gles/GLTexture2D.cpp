//
//  GLTexture2d.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#include "GLTexture2D.h"
#include "MathUtil/BitHacks.h"

NAMESPACE_RENDERCORE_BEGIN

GLTexture2D::GLTexture2D(const TextureDescriptor& des) : Texture2D(des)
{
    m_glTextureID = 0;
    m_textureDes = des;
    m_glesFormat = GetTransferFormatGLES(des.format);
    
    if (0 == des.width || 0 == des.height)
    {
        assert(false);
    }
}

GLTexture2D::~GLTexture2D()
{
    if (m_glTextureID != 0)
    {
        glDeleteTextures(1, &m_glTextureID);
    }
}

void GLTexture2D::allocMemory()
{
    if (m_glTextureID == 0)
    {
        glGenTextures(1, &m_glTextureID);
    }
    
    if (m_glTextureID == 0)
    {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_glTextureID);
    
    GLuint width = m_textureDes.width;
    GLuint height = m_textureDes.height;
    
    glTexImage2D(GL_TEXTURE_2D, 0, m_glesFormat.internalFormat, width, height, 0, m_glesFormat.format, m_glesFormat.dataType, nullptr);
}

/**
  set image data

 @param imageData image data
 */
void GLTexture2D::setTextureData(const uint8_t* imageData)
{
    if (imageData == nullptr)
    {
        return;
    }
    
    if (m_glTextureID == 0)
    {
        glGenTextures(1, &m_glTextureID);
    }
    if (m_glTextureID == 0)
    {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_glTextureID);
    
    GLuint width = m_textureDes.width;
    GLuint height = m_textureDes.height;
    
    //opengl纹理默认一行是4字节对齐的，不是4字节对齐的话会扭曲，需要设置对齐的字节数
    int bytesPerRow = width * m_textureDes.bytesPerRow;
    if (bytesPerRow % 4)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, m_glesFormat.internalFormat, width, height, 0, m_glesFormat.format, m_glesFormat.dataType, imageData);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    //创建mipmap纹理需要2的N次方
    if (m_textureDes.mipmaped && mathutil::IsPowerOfTwo(width) && mathutil::IsPowerOfTwo(height))
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

/**
  更新纹理数据
 
 @param rect 更新纹理区域
 @param mipMapLevel 纹理等级
 @param imageData 跟新数据
 */
void GLTexture2D::replaceRegion(const Rect2D& rect, const uint8_t* imageData, uint32_t mipMapLevel)
{
    if (nullptr == imageData)
    {
        return;
    }
    
    int width = rect.width;
    int height = rect.height;
    
    //opengl纹理默认一行是4字节对齐的，不是4字节对齐的话会扭曲，需要设置对齐的字节数
    int bytesPerRow = width * m_textureDes.bytesPerRow;
    if (0 == bytesPerRow % 4)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    else
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    
    //assert the current texture id is valid
    if (m_glTextureID == 0 || !glIsTexture(m_glTextureID))
    {
        glGenTextures(1, &m_glTextureID);
        
        //需要先分配内存
        glBindTexture(GL_TEXTURE_2D, m_glTextureID);
        
        glTexImage2D(GL_TEXTURE_2D, mipMapLevel, m_glesFormat.internalFormat, m_textureDes.width, m_textureDes.height, 0, m_glesFormat.format, m_glesFormat.dataType, imageData);
    }
    
    else
    {
        GLuint offsetx = rect.offsetX;
        GLuint offsety = rect.offsetY;
        
        //assert the rect is valid
        if ((offsetx+width) > m_textureDes.width || (offsety+height) > m_textureDes.height )
        {
            return;
        }
        glBindTexture(GL_TEXTURE_2D, m_glTextureID);
        
        glTexSubImage2D(GL_TEXTURE_2D, mipMapLevel, offsetx, offsety, width, height, m_glesFormat.format, m_glesFormat.dataType, imageData);
    }
    
    //创建mipmap纹理需要2的N次方
    if (m_textureDes.mipmaped && mathutil::IsPowerOfTwo(width) && mathutil::IsPowerOfTwo(height))
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

/**
 纹理是否有效

 @return ture or false
 */
bool GLTexture2D::isValid() const
{
    return m_glTextureID && glIsTexture(m_glTextureID);
}

void GLTexture2D::apply(GLuint nTextureUint)
{
    assert(m_glTextureID != 0);
    if (m_glTextureID == 0)
    {
        return;
    }
    
    GLenum textUnit = GL_TEXTURE0 + nTextureUint;
    glActiveTexture(textUnit);
    
    glBindTexture(GL_TEXTURE_2D, m_glTextureID);
}

void GLTexture2D::unbind(GLuint nTextureUint)
{
    GLenum textUnit = GL_TEXTURE0 + nTextureUint;
    glActiveTexture(textUnit);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

NAMESPACE_RENDERCORE_END
