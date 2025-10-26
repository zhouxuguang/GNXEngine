//
//  GLTextureSampler.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_GL_TEXTURE_SAMPLER_INCLUDE_HPP_JKL
#define GNX_ENGINE_GL_TEXTURE_SAMPLER_INCLUDE_HPP_JKL

#include "GLRenderDefine.h"
#include "GLGarbgeFactory.h"
#include "TextureSampler.h"
#include "gl2ext.h"

NAMESPACE_RENDERCORE_BEGIN

struct GLSamplerDescriptor
{
    GLint  filterMag      = GL_NEAREST;    // NEAREST
    GLint  filterMin      = GL_NEAREST;    // NEAREST
    GLint  wrapS          = GL_CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    GLint  wrapT          = GL_CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    GLenum wrapR          = GL_CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    uint8_t anisotropyLog2 = 0;    // 0
    GLint compareMode  = GL_TEXTURE_COMPARE_MODE_EXT;    // NONE
    GLint compareFunc  = GL_LEQUAL;    // LE
};

class GLTextureSampler : public TextureSampler
{
    
public:
    GLTextureSampler(GLGarbgeFactoryWeakPtr garbagyFactory, const SamplerDescriptor& des) noexcept;
    
    ~GLTextureSampler() noexcept;
    
    /**
     Get the sampler descriptor for opengl es platform

     @return GLTextureSampler
     */
    inline const GLSamplerDescriptor& getGLTextureSampler() noexcept {return m_glSamplerDescriptor;};
    
    
    /**
     Call openGL function to apply the sampler
     */
    void apply(GLuint textureUnit) noexcept;
    
    void setSamplerInvalid(){ m_samplerID = 0; }
    
    /**
     解除绑定
     */
    static void unbind(GLuint nTextureUint) noexcept;
    
private:
    
    void transToGLSamplerDescriptor(const SamplerDescriptor& des) noexcept;
    
    GLint transToGLAdressMode(SamplerWrapMode mode) noexcept;
    
    SamplerDescriptor m_samplerParams;
    
    GLSamplerDescriptor m_glSamplerDescriptor;
    
    GLuint m_samplerID;
    
    GLGarbgeFactoryWeakPtr m_garbagyFactory;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_TEXTURE_SAMPLER_INCLUDE_HPP_JKL */
