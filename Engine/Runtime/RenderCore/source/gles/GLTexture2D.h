//
//  GLTexture2d.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_GL_TEXTURE2D_INCLUDE_HPP
#define GNX_ENGINE_GL_TEXTURE2D_INCLUDE_HPP

#include "Texture2D.h"
#include "GLRenderDefine.h"
#include "RenderDescriptor.h"
#include "GLES3Untily.h"

NAMESPACE_RENDERCORE_BEGIN

class GLTexture2D : public Texture2D
{
public:
    GLTexture2D(const TextureDescriptor& des);
    
    ~GLTexture2D();
    
    void allocMemory();
    
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
    
    /**
     绑定纹理数据
     */
    void apply(GLuint nTextureUint);
    
    /**
     解除绑定
     */
    static void unbind(GLuint nTextureUint);
    
    GLuint getTextureID() const
    {
        return m_glTextureID;
    }
    
private:
    GLuint m_glTextureID;
    TransferFormatGLES m_glesFormat;
    
    TextureDescriptor m_textureDes;
};

typedef std::shared_ptr<GLTexture2D> GLTexture2DPtr;

NAMESPACE_RENDERCORE_END


#endif /* GNX_ENGINE_GL_TEXTURE2D_INCLUDE_HPP */
