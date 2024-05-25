//
//  MTLUniformBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLUniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLUniformBuffer::MTLUniformBuffer(id<MTLDevice> device, uint32_t size)
{
    if (size > 4096)
    {
        mBuffer = std::make_unique<MTLBufferBase>(device, size, StorageModeShared);
        mIsBuufer = true;
    }
    else
    {
        mBufferData.resize(size);
    }
}

MTLUniformBuffer::~MTLUniformBuffer()
{
    mBufferData.clear();
}

void MTLUniformBuffer::setData(const void* data, uint32_t offset, uint32_t dataSize)
{
    if (mIsBuufer)
    {
        uint8_t* bufferData = (uint8_t*)mBuffer->getBufferData();
        memcpy(bufferData + offset, data, dataSize);
    }
    else
    {
        memcpy(mBufferData.data() + offset, data, dataSize);
    }
}

NAMESPACE_RENDERCORE_END
