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

uint32_t MTLVertexBuffer::GetBufferLength() const
{
    if (mBuffer)
    {
        // 修复：原实现漏写 return，导致恒返回 0（查询缓冲长度永远得到 0）
        return static_cast<uint32_t>(mBuffer->getBufferLength());
    }
    return 0;
}

void* MTLVertexBuffer::MapBufferData() const
{
    if (mBuffer)
    {
        return mBuffer->getBufferData();
    }
    return nullptr;
}

void MTLVertexBuffer::UnmapBufferData(void* bufferData) const
{
    //
}

bool MTLVertexBuffer::IsValid() const
{
    return false;
}

NAMESPACE_RENDERCORE_END
