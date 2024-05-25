//
//  GLBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/26.
//

#include "GLBuffer.h"
#include "gl3stub.h"

NAMESPACE_RENDERCORE_BEGIN
    
GLBuffer::GLBuffer(GLenum bufferType, const void* pData, uint32_t size, StorageMode mode)
{
    m_bufferType = bufferType;
    m_storageMode = mode;
    if (mode == StorageModeShared)
    {
        m_bufferData.resize(size);
        memcpy(m_bufferData.data(), pData, size);
    }
    
    else
    {
        glGenBuffers(1, &m_bufferId);
        glBindBuffer(bufferType, m_bufferId);
        
        glBufferData(bufferType, size, pData, GL_STATIC_DRAW);
        glBindBuffer(bufferType, 0);
    }
    
    m_bufferSize = size;
}

GLBuffer::GLBuffer(GLenum bufferType, uint32_t size, StorageMode mode)
{
    m_bufferType = bufferType;
    m_storageMode = mode;
    if (mode == StorageModeShared)
    {
        m_bufferData.resize(size);
    }
    
    else
    {
        glGenBuffers(1, &m_bufferId);
        glBindBuffer(bufferType, m_bufferId);
        
        glBufferData(bufferType, size, nullptr, GL_STATIC_DRAW);
        glBindBuffer(bufferType, 0);
    }
    
    m_bufferSize = size;
}

GLBuffer::~GLBuffer()
{
    //
}

void* GLBuffer::mapBufferData()
{
    if (m_storageMode == StorageModeShared)
    {
        return m_bufferData.data();
    }
    
    else
    {
        Bind();
        void *data = nullptr;
        
        if (OpenGLESContext::isSupportGLES30())
        {
            data = glMapBufferRange(m_bufferType, 0, (uint32_t)m_bufferData.size(), GL_MAP_WRITE_BIT);
        }
        else
        {
            data = glMapBufferOES(m_bufferType, GL_WRITE_ONLY_OES);
        }
        UnBind();
        return data;
        
    }
    
}

void GLBuffer::unmapBufferData()
{
    if (m_storageMode == StorageModeShared)
    {
        return;
    }
    
    Bind();
    if (OpenGLESContext::isSupportGLES30())
    {
        glUnmapBuffer(m_bufferType);
    }
    else
    {
        glUnmapBufferOES(m_bufferType);
    }
    UnBind();
}
    
void GLBuffer::Bind()
{
    glBindBuffer(m_bufferType, m_bufferId);
}
    
void GLBuffer::UnBind()
{
    glBindBuffer(m_bufferType, 0);
}

bool GLBuffer::isValid() const
{
    if (m_storageMode == StorageModeShared)
    {
        return !m_bufferData.empty();
    }
    
    return glIsBuffer(m_bufferId) && m_bufferId != 0;
}

void GLBuffer::bindVertexData()
{
    //
}

void GLBuffer::setData(const void* data, uint32_t offset, uint32_t dataSize)
{
    if (m_storageMode == StorageModeShared)
    {
        memcpy(m_bufferData.data() + offset, data, dataSize);
    }
    
    else
    {
        glBindBuffer(m_bufferType, m_bufferId);
        glBufferSubData(m_bufferType, offset, dataSize, data);
        glBindBuffer(m_bufferType, 0);
    }
}

NAMESPACE_RENDERCORE_END
