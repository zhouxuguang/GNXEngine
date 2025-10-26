//
//  GLIndexBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/27.
//

#include "GLIndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

GLIndexBuffer::GLIndexBuffer(IndexType indexType, const void* pData, uint32_t size) : IndexBuffer(indexType, pData, size)
{
    m_buffer = std::make_unique<GLBuffer>(GL_ELEMENT_ARRAY_BUFFER, pData, size, StorageModePrivate);
    m_indexType = indexType;
}

GLIndexBuffer::~GLIndexBuffer()
{
    m_buffer.reset();
}

void* GLIndexBuffer::mapBufferData() const
{
    return m_buffer->mapBufferData();
}

/**
 解除buffer数据
 */
void GLIndexBuffer::unmapBufferData(void* bufferData) const
{
    m_buffer->unmapBufferData();
}

void GLIndexBuffer::Bind()
{
    m_buffer->Bind();
}

void GLIndexBuffer::UnBind()
{
    m_buffer->UnBind();
}

NAMESPACE_RENDERCORE_END
