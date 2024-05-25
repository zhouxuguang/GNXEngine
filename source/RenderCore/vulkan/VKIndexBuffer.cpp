//
//  VKIndexBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKIndexBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKIndexBuffer::VKIndexBuffer(VulkanContextPtr context, IndexType indexType, const void* pData, uint32_t dataLen)
    :IndexBuffer(indexType, pData, dataLen)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, dataLen,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);

    void *data = nullptr;
    vmaMapMemory(context->vmaAllocator, mAllocation, &data);
    memcpy(data, pData, dataLen);
    vmaUnmapMemory(context->vmaAllocator, mAllocation);
    
    mBufferLength = (uint32_t)dataLen;
    mIndexType = indexType;
}

VKIndexBuffer::~VKIndexBuffer()
{
}

NAMESPACE_RENDERCORE_END
