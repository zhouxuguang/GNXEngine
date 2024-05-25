//
//  GLBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/26.
//

#ifndef GNX_ENGINE_GLBUFFER_INCLUDE_JGJGDFJ_H
#define GNX_ENGINE_GLBUFFER_INCLUDE_JGJGDFJ_H

#include <stdio.h>
#include <vector>
#include "GLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN


//opengl。buffer的管理类

class GLBuffer
{
public:
    GLBuffer(GLenum bufferType, const void* pData, uint32_t size, StorageMode mode);
    
    GLBuffer(GLenum bufferType, uint32_t size, StorageMode mode);
    
    ~GLBuffer();
    
    void Bind();
    
    void UnBind();
    
    void* mapBufferData();
    
    void unmapBufferData();
    
    uint32_t getBufferSize() const
    {
        if (m_storageMode == StorageModeShared)
        {
            return (uint32_t)m_bufferData.size();
        }
        return m_bufferSize;
    }
    
    bool isValid() const;
    
    StorageMode getStorageMode() const
    {
        return m_storageMode;
    }
    
    //进行数据的绑定
    void bindVertexData();
    
    void setData(const void* data, uint32_t offset, uint32_t dataSize);
    
    GLuint getBufferID() const
    {
        return m_bufferId;
    }
    
private:
    std::vector<uint8_t> m_bufferData;
    GLuint m_bufferId = 0;
    
    GLenum m_bufferType;
    StorageMode m_storageMode;
    uint32_t m_bufferSize = 0;
};

typedef std::unique_ptr<GLBuffer> GLBufferUniquePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLBUFFER_INCLUDE_JGJGDFJ_H */
