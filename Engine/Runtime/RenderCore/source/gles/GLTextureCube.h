//
//  GLTextureCube.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/6.
//

#ifndef GNX_ENGINE_GLTEXTURE_CUBE_INCLUDE_H
#define GNX_ENGINE_GLTEXTURE_CUBE_INCLUDE_H

#include "TextureCube.h"
#include "GLRenderDefine.h"
#include "RenderDescriptor.h"
#include "GLES3Untily.h"

NAMESPACE_RENDERCORE_BEGIN

class GLTextureCube : public TextureCube
{
public:
    GLTextureCube(const std::vector<TextureDescriptor>& desArray);
    
    ~GLTextureCube();
    
    /**
      set image data

     @param imageData image data
     */
    virtual void setTextureData(CubemapFace cubeFace, uint32_t imageSize, const uint8_t* imageData);
    
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
    
private:
    GLuint m_glTextureID;
    bool m_isCompressedTexture = false;
    
    std::vector<TransferFormatGLES> m_innerFormatArray;
    std::vector<TextureDescriptor> m_textureDesArray;
};

typedef std::shared_ptr<GLTextureCube> GLTextureCubePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLTEXTURE_CUBE_INCLUDE_H */
