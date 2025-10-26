//
//  GLTextureSampler.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#include "GLTextureSampler.h"
#include "gl3stub.h"

NAMESPACE_RENDERCORE_BEGIN

GLTextureSampler::~GLTextureSampler() noexcept
{
    auto gargage = m_garbagyFactory.lock();
    if (gargage)
    {
        gargage->postSampler(m_samplerID);
    }
    m_samplerID = 0;
}

GLTextureSampler::GLTextureSampler(std::weak_ptr<GLGarbgeFactory> garbagyFactory, const SamplerDescriptor& des) noexcept:TextureSampler(des)
{
    garbagyFactory = garbagyFactory;
    m_samplerID = 0;
    transToGLSamplerDescriptor(des);
    m_samplerParams = des;
}

void GLTextureSampler::transToGLSamplerDescriptor(const SamplerDescriptor &des) noexcept
{
    switch (des.filterMag)
    {
        case SamplerMagFilter::MAG_LINEAR:
            m_glSamplerDescriptor.filterMag = GL_LINEAR;
            break;
        case SamplerMagFilter::MAG_NEAREST:
            m_glSamplerDescriptor.filterMag = GL_NEAREST;
            break;
        default: //should not go here
            break;
    }
    
    switch (des.filterMin)
    {
        case SamplerMinFilter::MIN_LINEAR:
            m_glSamplerDescriptor.filterMin = GL_LINEAR;
            break;
        case SamplerMinFilter::MIN_NEAREST:
            m_glSamplerDescriptor.filterMin = GL_NEAREST;
            break;
        case SamplerMinFilter::MIN_LINEAR_MIPMAP_LINEAR:
            m_glSamplerDescriptor.filterMin = GL_LINEAR_MIPMAP_LINEAR;
            break;
        case SamplerMinFilter::MIN_LINEAR_MIPMAP_NEAREST:
            m_glSamplerDescriptor.filterMin = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case SamplerMinFilter::MIN_NEAREST_MIPMAP_LINEAR:
            m_glSamplerDescriptor.filterMin = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case SamplerMinFilter::MIN_NEAREST_MIPMAP_NEAREST:
            m_glSamplerDescriptor.filterMin = GL_NEAREST_MIPMAP_NEAREST;
            break;
        default:
            break;
    }
    m_glSamplerDescriptor.wrapR = transToGLAdressMode(des.wrapR);
    m_glSamplerDescriptor.wrapS = transToGLAdressMode(des.wrapS);
    m_glSamplerDescriptor.wrapT = transToGLAdressMode(des.wrapT);
    m_glSamplerDescriptor.anisotropyLog2 = des.anisotropyLog2;
}

GLint GLTextureSampler::transToGLAdressMode(SamplerWrapMode mode) noexcept
{
    GLint addressMode = GL_CLAMP_TO_EDGE;
    switch (mode) {
        case SamplerWrapMode::CLAMP_TO_EDGE:
            addressMode = GL_CLAMP_TO_EDGE;
            break;
        case SamplerWrapMode::REPEAT:
            addressMode = GL_REPEAT;
            break;
        case SamplerWrapMode::MIRRORED_REPEAT:
            addressMode = GL_MIRRORED_REPEAT;
            break;
        default:
            break;
    }
    return addressMode;
}

void GLTextureSampler::apply(GLuint textureUnit) noexcept
{
    if (OpenGLESContext::isSupportGLES30())
    {
        if (m_samplerID == 0 || !glIsSampler(m_samplerID))
        {
            glGenSamplers(1, &m_samplerID);
            glBindSampler(textureUnit, m_samplerID);
            
            glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, m_glSamplerDescriptor.filterMin);
            glSamplerParameteri(m_samplerID, GL_TEXTURE_MAG_FILTER, m_glSamplerDescriptor.filterMag);
            glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_S, m_glSamplerDescriptor.wrapS);
            glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_T, m_glSamplerDescriptor.wrapT);
            glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_R, m_glSamplerDescriptor.wrapR);
            
            //这个在iOS上不支持的原因还需要查，各向异性采样
            //glSamplerParameteri(m_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_glSamplerDescriptor.anisotropyLog2);
        }
        glBindSampler(textureUnit, m_samplerID);
    }
    
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_glSamplerDescriptor.filterMag);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_glSamplerDescriptor.filterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_glSamplerDescriptor.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_glSamplerDescriptor.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, m_glSamplerDescriptor.wrapR);
    }
}

void GLTextureSampler::unbind(GLuint nTextureUint) noexcept
{
    if (OpenGLESContext::isSupportGLES30())
    {
        glBindSampler(nTextureUint, 0);
    }
}

NAMESPACE_RENDERCORE_END
