//
//  MTLIndexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_INDEX_BUFFER_INCLUDE_EGG
#define GNX_ENGINE_MTL_INDEX_BUFFER_INCLUDE_EGG

#include "MTLBufferBase.h"
#include "IndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLIndexBuffer : public IndexBuffer
{
public:
    MTLIndexBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue,
                   IndexType indexType, const void* pData, uint32_t dataLen);
    
    ~MTLIndexBuffer();
    
    id<MTLBuffer> getMTLBuffer()
    {
        if (mBuffer)
        {
            return mBuffer->getMTLBuffer();
        }
        return nullptr;
    }
    
    IndexType getIndexType()
    {
        return mIndexType;
    }
    
private:
    MTLBufferBasePtr mBuffer = nullptr;
    IndexType mIndexType;
};

typedef std::shared_ptr<MTLIndexBuffer> MTLIndexBufferPtr;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_INDEX_BUFFER_INCLUDE_EGG */
