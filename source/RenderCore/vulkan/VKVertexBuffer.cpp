//
//  VKVertexBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKVertexBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKVertexBuffer::VKVertexBuffer(VulkanContextPtr context,
        size_t len, StorageMode mode) : mContext(context)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (mode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mode, len,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memType, mBuffer, mAllocation, nullptr);

    mStorageMode = mode;
    mBufferLength = (uint32_t)len;
}

VKVertexBuffer::VKVertexBuffer(VulkanContextPtr context, const void* buffer, size_t size, StorageMode mode) : mContext(context)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (mode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mode, size,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);

    // private模式的还需要继续处理
    void *data = nullptr;
    vmaMapMemory(context->vmaAllocator, mAllocation, &data);
    memcpy(data, buffer, size);
    vmaUnmapMemory(context->vmaAllocator, mAllocation);
    
    mStorageMode = mode;
    mBufferLength = (uint32_t)size;
}

VKVertexBuffer::~VKVertexBuffer()
{
    vmaDestroyBuffer(mContext->vmaAllocator, mBuffer, mAllocation);
}

uint32_t VKVertexBuffer::getBufferLength() const
{
    return mBufferLength;
}

void* VKVertexBuffer::mapBufferData() const
{
    if (mStorageMode == StorageModeShared)
    {
        void *data = nullptr;
        vmaMapMemory(mContext->vmaAllocator, mAllocation, &data);
        return data;
    }
    else
    {
        return NULL;
    }
}

void VKVertexBuffer::unmapBufferData(void* bufferData) const
{
    vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
}

bool VKVertexBuffer::isValid() const
{
    return mBuffer != VK_NULL_HANDLE;
}

VkBuffer VKVertexBuffer::GetGpuBuffer() const
{
    return mBuffer;
}

NAMESPACE_RENDERCORE_END
