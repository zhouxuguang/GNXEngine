//
//  GLUniformBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/27.
//

#include "GLUniformBuffer.h"
#include "gl3stub.h"

NAMESPACE_RENDERCORE_BEGIN

GLUniformBuffer::GLUniformBuffer(uint32_t size)
{
    m_buffer = std::make_unique<GLBuffer>(GL_UNIFORM_BUFFER, size, StorageModePrivate);
}

GLUniformBuffer::~GLUniformBuffer()
{
    m_buffer.reset();
}

void GLUniformBuffer::setData(const void* data, uint32_t offset, uint32_t dataSize)
{
    m_buffer->setData(data, offset, dataSize);
    m_bUpdate = true;
}

void GLUniformBuffer::apply(uint32_t uboSize, uint32_t bindingPoint)
{
//    GLint nBlockDataSize = 0;
//    glGetActiveUniformBlockiv(program, 0, GL_UNIFORM_BLOCK_DATA_SIZE, &nBlockDataSize);
   // m_buffer->Bind();
    //glBufferData(GL_UNIFORM_BUFFER, nBlockDataSize, NULL, GL_DYNAMIC_DRAW);
    
    //判断m_buffer->getBufferSize()是否和从program中的datasize是否一样
//    if (m_buffer->getBufferSize() != uboSize)
//    {
//        assert(false);
//        return;
//    }
    
    if (m_bUpdate)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer->getBufferID());
    }
}

NAMESPACE_RENDERCORE_END
