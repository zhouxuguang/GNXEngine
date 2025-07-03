//
//  VertexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/26.
//

#ifndef GNX_ENGINE_VERTEX_BUFFER_INCLUDE
#define GNX_ENGINE_VERTEX_BUFFER_INCLUDE

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class VertexBuffer
{
public:
    VertexBuffer();
    
    virtual ~VertexBuffer();
    
    /**
     获取Buffer的长度
     
     @return buffer长度,单位btye
     */
    virtual uint32_t GetBufferLength() const = 0;
    
    /**
     映射buffer数据
     
     @return buffer数据起始地址
     */
    virtual void* MapBufferData() const = 0;
    
    /**
     解除buffer数据
     */
    virtual void UnmapBufferData(void* bufferData) const = 0;
    
    virtual bool IsValid() const = 0;
};


typedef std::shared_ptr<VertexBuffer> VertexBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VERTEX_BUFFER_INCLUDE */
