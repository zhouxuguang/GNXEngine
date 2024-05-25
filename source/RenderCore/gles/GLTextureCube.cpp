//
//  GLTextureCube.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/6.
//

#include "GLTextureCube.h"

NAMESPACE_RENDERCORE_BEGIN

GLTextureCube::GLTextureCube(const std::vector<TextureDescriptor>& desArray) : TextureCube(desArray)
{
    if (desArray.size() != 6)
    {
        return;
    }
    
    m_innerFormatArray.resize(desArray.size());
    m_textureDesArray.resize(desArray.size());
    
    for (size_t i = 0; i < desArray.size(); i ++)
    {
        m_innerFormatArray[i] = GetTransferFormatGLES(desArray[i].format);
        m_textureDesArray[i] = desArray[i];
    }
    
    m_isCompressedTexture = IsAnyCompressedTextureFormat(desArray[0].format);
}

GLTextureCube::~GLTextureCube()
{
    m_innerFormatArray.clear();
    m_textureDesArray.clear();
}

/**
  set image data

 @param imageData image data
 */
void GLTextureCube::setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData)
{
    if (nullptr == imageData /*|| 0 == imageSize*/)
    {
        return;
    }
    
    if (!isValid())
    {
        glGenTextures(1, &m_glTextureID);
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTextureID);
    
    int index = cubeFace;
    if (m_isCompressedTexture)
    {
        glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, m_innerFormatArray[index].internalFormat, m_textureDesArray[index].width, m_textureDesArray[index].height, 0, imageSize, imageData);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, m_innerFormatArray[index].internalFormat,
                     m_textureDesArray[index].width, m_textureDesArray[index].height, 0, m_innerFormatArray[index].format, m_innerFormatArray[index].dataType, imageData);
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

/**
 纹理是否有效

 @return ture or false
 */
bool GLTextureCube::isValid() const
{
    return m_glTextureID && glIsTexture(m_glTextureID);
}

/**
 绑定纹理数据
 */
void GLTextureCube::apply(GLuint nTextureUint)
{
    assert(m_glTextureID != 0);
    if (m_glTextureID == 0)
    {
        return;
    }
    
    GLenum textUnit = GL_TEXTURE0 + nTextureUint;
    glActiveTexture(textUnit);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_glTextureID);
}

/**
 解除绑定
 */
void GLTextureCube::unbind(GLuint nTextureUint)
{
    GLenum textUnit = GL_TEXTURE0 + nTextureUint;
    glActiveTexture(textUnit);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

NAMESPACE_RENDERCORE_END
