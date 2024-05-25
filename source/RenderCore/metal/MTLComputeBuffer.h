//
//  MTLComputeBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_MTL_COMPUTE_DJJKGJ_BUFFER_INCLUDE
#define GNX_ENGINE_MTL_COMPUTE_DJJKGJ_BUFFER_INCLUDE

#include "MTLRenderDefine.h"
#include "ComputeBuffer.h"
#include "MTLVertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLComputeBuffer : public MTLVertexBuffer, public ComputeBuffer
{
public:
    MTLComputeBuffer(id<MTLDevice> device, size_t len, StorageMode mode) : MTLVertexBuffer(device, len, mode)
    {
        
    }
    
    MTLComputeBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const void* buffer, size_t size, StorageMode mode)
    : MTLVertexBuffer(device, commandQueue, buffer, size, mode)
    {
        //
    }
    
    ~MTLComputeBuffer(){}
    
    virtual uint32_t getBufferLength() const 
    {
        return MTLVertexBuffer::getBufferLength();
    }
    
    virtual void* mapBufferData() const 
    {
        return MTLVertexBuffer::mapBufferData();
    }
    
    virtual void unmapBufferData(void* bufferData) const 
    {
        return MTLVertexBuffer::unmapBufferData(bufferData);
    }
    
    virtual bool isValid() const 
    {
        return MTLVertexBuffer::isValid();
    }
    
};

typedef std::shared_ptr<MTLComputeBuffer> MTLComputeBufferPtr;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMPUTE_DJJKGJ_BUFFER_INCLUDE */
