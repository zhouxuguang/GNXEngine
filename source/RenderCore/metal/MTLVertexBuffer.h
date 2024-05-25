//
//  MTLVertexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_VERTEX_BUFFER_INCLUDE_HFF
#define GNX_ENGINE_MTL_VERTEX_BUFFER_INCLUDE_HFF

#include "MTLBufferBase.h"
#include "VertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLVertexBuffer : public VertexBuffer
{
public:
    MTLVertexBuffer(id<MTLDevice> device, size_t len, StorageMode mode);
    
    MTLVertexBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const void* buffer, size_t size, StorageMode mode);
    
    ~MTLVertexBuffer();
    
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
    
    id<MTLBuffer> getMTLBuffer()
    {
        if (mBuffer)
        {
            return mBuffer->getMTLBuffer();
        }
        return nullptr;
    }
    
private:
    MTLBufferBasePtr mBuffer = nullptr;
};

typedef std::shared_ptr<MTLVertexBuffer> MTLVertexBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_VERTEX_BUFFER_INCLUDE_HFF */
