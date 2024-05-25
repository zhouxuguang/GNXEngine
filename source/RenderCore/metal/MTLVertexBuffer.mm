//
//  MTLVertexBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLVertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLVertexBuffer::MTLVertexBuffer(id<MTLDevice> device, size_t len, StorageMode mode)
{
    mBuffer = std::make_unique<MTLBufferBase>(device, len, mode);
}

MTLVertexBuffer::MTLVertexBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const void* buffer, size_t size, StorageMode mode)
{
    mBuffer = std::make_unique<MTLBufferBase>(device, commandQueue, buffer, size, mode);
}

MTLVertexBuffer::~MTLVertexBuffer()
{
    //
}

uint32_t MTLVertexBuffer::getBufferLength() const
{
    if (mBuffer)
    {
        mBuffer->getBufferLength();
    }
    return 0;
}

void* MTLVertexBuffer::mapBufferData() const
{
    if (mBuffer)
    {
        return mBuffer->getBufferData();
    }
    return nullptr;
}

void MTLVertexBuffer::unmapBufferData(void* bufferData) const
{
    //
}

bool MTLVertexBuffer::isValid() const
{
    return false;
}

NAMESPACE_RENDERCORE_END
