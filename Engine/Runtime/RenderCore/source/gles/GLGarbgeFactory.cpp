//
//  GLGarbgeFactory.cpp
//  RenderEngine
//
//  Created by Zhou,Xuguang on 2019/4/30.
//  Copyright © 2019年 Zhou,Xuguang. All rights reserved.
//

#include "GLGarbgeFactory.h"
#include "gl3stub.h"

NAMESPACE_RENDERCORE_BEGIN

GLGarbgeFactory::GLGarbgeFactory()
{
    m_vecTextureIDs.clear();
    m_vecBufferIDs.clear();
    m_vecSamplerIDs.clear();
    m_vecProgramIDs.clear();
}

GLGarbgeFactory::~GLGarbgeFactory()
{
    m_vecTextureIDs.clear();
    m_vecBufferIDs.clear();
    m_vecSamplerIDs.clear();
    m_vecProgramIDs.clear();
}

void GLGarbgeFactory::postTexture(GLuint textureID)
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    m_vecTextureIDs.push_back(textureID);
}

void GLGarbgeFactory::postBuffer(GLuint bufferID)
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    m_vecBufferIDs.push_back(bufferID);
}

void GLGarbgeFactory::postSampler(GLuint samplerID)
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    m_vecSamplerIDs.push_back(samplerID);
}

void GLGarbgeFactory::postShader(GLuint programID)
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    m_vecProgramIDs.push_back(programID);
}

void GLGarbgeFactory::gc()
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    
    //删除缓冲区对象
    if (!m_vecBufferIDs.empty())
    {
        glDeleteBuffers((GLsizei)m_vecBufferIDs.size(), m_vecBufferIDs.data());
        
        std::vector<GLuint> temp;
        m_vecBufferIDs.swap(temp);
    }
    
    //删除采样器对象  3.0支持
    if (OpenGLESContext::isSupportGLES30())
    {
        if (!m_vecSamplerIDs.empty())
        {
            glDeleteSamplers((GLsizei)m_vecSamplerIDs.size(), m_vecSamplerIDs.data());
        }
        
        std::vector<GLuint> temp;
        m_vecSamplerIDs.swap(temp);
    }
    
    //删除纹理对象
    if (!m_vecTextureIDs.empty())
    {
        glDeleteTextures((GLsizei)m_vecTextureIDs.size(), m_vecTextureIDs.data());
        
        std::vector<GLuint> temp;
        m_vecTextureIDs.swap(temp);
    }
    
    //删除程序对象
    for (size_t i = 0; i < m_vecProgramIDs.size(); i ++)
    {
        glDeleteProgram(m_vecProgramIDs[i]);
    }
    
    std::vector<GLuint> temp;
    m_vecProgramIDs.swap(temp);
    
}

void GLGarbgeFactory::clear()
{
    std::lock_guard<std::mutex> lock_guard(m_ResourceLock);
    m_vecBufferIDs.clear();
    m_vecSamplerIDs.clear();
    m_vecTextureIDs.clear();
    m_vecProgramIDs.clear();
}

NAMESPACE_RENDERCORE_END
