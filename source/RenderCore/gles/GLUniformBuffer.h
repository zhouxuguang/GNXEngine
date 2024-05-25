//
//  GLUniformBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/27.
//

#ifndef GNE_RENDERCORE_GL_UNIFORM_BUFFER_INCLUDE_SFDJF_H
#define GNE_RENDERCORE_GL_UNIFORM_BUFFER_INCLUDE_SFDJF_H

#include "GLRenderDefine.h"
#include "GLBuffer.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class GLUniformBuffer : public UniformBuffer
{
public:
    explicit GLUniformBuffer(uint32_t size);
    
    ~GLUniformBuffer();
    
    virtual void setData(const void* data, uint32_t offset, uint32_t dataSize);
    
    void apply(uint32_t uboSize, uint32_t bindingPoint);
    
private:
    GLBufferUniquePtr m_buffer = nullptr;
    bool m_bUpdate = false;
};

typedef std::shared_ptr<GLUniformBuffer> GLUniformBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNE_RENDERCORE_GL_UNIFORM_BUFFER_INCLUDE_SFDJF_H */
