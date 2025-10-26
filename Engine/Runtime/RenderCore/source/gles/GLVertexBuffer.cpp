//
//  GLVertexBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/26.
//

#include "GLVertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

GLVertexBuffer::GLVertexBuffer(const void* pData, uint32_t size, StorageMode mode)
{
    m_buffer = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER, pData, size, mode);
}

GLVertexBuffer::GLVertexBuffer(uint32_t size, StorageMode mode)
{
    m_buffer = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER, size, mode);
}

GLVertexBuffer::~GLVertexBuffer()
{
    m_buffer.reset();
}

uint32_t GLVertexBuffer::getBufferLength() const
{
    return m_buffer->getBufferSize();
}

void* GLVertexBuffer::mapBufferData() const
{
    return m_buffer->mapBufferData();
}

void GLVertexBuffer::unmapBufferData(void* bufferData) const
{
    m_buffer->unmapBufferData();
}

bool GLVertexBuffer::isValid() const
{
    return m_buffer && m_buffer->isValid();
}

void GLVertexBuffer::bindData()
{
    //
}

void GLVertexBuffer::Bind()
{
    m_buffer->Bind();
}

void GLVertexBuffer::UnBind()
{
    m_buffer->UnBind();
}

NAMESPACE_RENDERCORE_END
