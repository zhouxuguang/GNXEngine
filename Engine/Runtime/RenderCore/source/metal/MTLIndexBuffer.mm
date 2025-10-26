//
//  MTLIndexBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLIndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLIndexBuffer::MTLIndexBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue,
               IndexType indexType, const void* pData, uint32_t dataLen)
                : IndexBuffer(indexType, pData, dataLen)
{
    mBuffer = std::make_unique<MTLBufferBase>(device, commandQueue, pData, dataLen, StorageModePrivate);
    mIndexType = indexType;
}

MTLIndexBuffer::~MTLIndexBuffer()
{
    //
}

NAMESPACE_RENDERCORE_END
