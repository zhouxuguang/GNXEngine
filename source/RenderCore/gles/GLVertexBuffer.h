//
//  GLVertexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/26.
//

#ifndef GNX_ENGINE_GL_VERTEXT_BUFFER_INCLUDE_H
#define GNX_ENGINE_GL_VERTEXT_BUFFER_INCLUDE_H

#include "GLBuffer.h"
#include "VertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class GLVertexBuffer : public VertexBuffer
{
public:
    GLVertexBuffer(const void* pData, uint32_t size, StorageMode mode);
    
    GLVertexBuffer(uint32_t size, StorageMode mode);
    
    ~GLVertexBuffer();
    
    /**
     获取Buffer的长度
     
     @return buffer长度,单位btye
     */
    virtual uint32_t getBufferLength() const;
    
    /**
     映射buffer数据
     
     @return buffer数据起始地址
     */
    virtual void* mapBufferData() const;
    
    /**
     解除buffer数据
     */
    virtual void unmapBufferData(void* bufferData) const;
    
    virtual bool isValid() const;
    
    bool isVBO() const
    {
        if (!m_buffer)
        {
            return false;
        }
        
        return m_buffer->getStorageMode() == StorageModePrivate;
    }
    
    //进行数据的绑定
    void bindData();
    
    void Bind();
    
    void UnBind();
    
private:
    GLBufferUniquePtr m_buffer = nullptr;
};

typedef std::shared_ptr<GLVertexBuffer> GLVertexBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_VERTEXT_BUFFER_INCLUDE_H */
