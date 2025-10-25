//
//  GLIndexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/27.
//

#ifndef GNX_ENGINE_GLINDEX_BUFFER_INCLUDE_H
#define GNX_ENGINE_GLINDEX_BUFFER_INCLUDE_H

#include "GLRenderDefine.h"
#include "GLBuffer.h"
#include "IndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class GLIndexBuffer : public IndexBuffer
{
public:
    GLIndexBuffer(IndexType indexType, const void* pData, uint32_t size);
    
    ~GLIndexBuffer();
    
    IndexType getIndexType() const
    {
        return m_indexType;
    }
    
    bool isValid() const
    {
        return m_buffer->isValid();
    }
    
    /**
     映射buffer数据
     
     @return buffer数据起始地址
     */
    virtual void* mapBufferData() const;
    
    /**
     解除buffer数据
     */
    virtual void unmapBufferData(void* bufferData) const;
    
    void Bind();
    
    void UnBind();
    
    bool isEBO() const
    {
        if (!m_buffer)
        {
            return false;
        }
        
        return m_buffer->getStorageMode() == StorageModePrivate;
    }
    
private:
    GLBufferUniquePtr m_buffer = nullptr;
    IndexType m_indexType;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLINDEX_BUFFER_INCLUDE_H */
