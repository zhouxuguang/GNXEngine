//
//  MTLUniformBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLUniformBuffer.h"
#include "Runtime/BaseLib/include/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

MTLUniformBuffer::MTLUniformBuffer(id<MTLDevice> device, uint32_t size)
    : mDevice(device)
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

void MTLUniformBuffer::SetData(const void* data, uint32_t offset, uint32_t dataSize)
{
    if (data == nullptr || dataSize == 0)
    {
        return;
    }

    const size_t capacity = mIsBuufer ? mBuffer->getBufferLength() : mBufferData.size();
    if (static_cast<uint64_t>(offset) + dataSize > capacity)
    {
        LOG_ERROR("[Metal] UniformBuffer::SetData out of range (offset=%u, dataSize=%u, capacity=%zu)",
                  offset, dataSize, capacity);
        return;
    }

    if (mIsBuufer)
    {
        uint8_t* bufferData = (uint8_t*)mBuffer->getBufferData();
        memcpy(bufferData + offset, data, dataSize);
    }
    else
    {
        memcpy(mBufferData.data() + offset, data, dataSize);
        UpdateNamedBuffer();
    }
}

void MTLUniformBuffer::SetName(const char* name)
{
    mDebugName = name ? name : "";
    if (mIsBuufer)
        mBuffer->SetName(mDebugName.c_str());
    else
        UpdateNamedBuffer();
}

void MTLUniformBuffer::UpdateNamedBuffer()
{
    if (mDebugName.empty() || mBufferData.empty())
    {
        mBuffer.reset();
        return;
    }

    // setBytes has no resource to label. Named small UBOs use an immutable
    // snapshot instead; recorded encoders retain it across later SetData calls.
    mBuffer = std::make_unique<MTLBufferBase>(mDevice, nullptr, mBufferData.data(),
                                             mBufferData.size(), StorageModeShared);
    mBuffer->SetName(mDebugName.c_str());
}

NAMESPACE_RENDERCORE_END
