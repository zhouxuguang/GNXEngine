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
    :IndexBuffer(indexType, pData, dataLen), mContext(context)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, dataLen,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);
    
    // 创建临时buffer用于上传数据
    VkBuffer stageBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, dataLen,
                                      bufferUsage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stageBuffer, allocation, nullptr);
    
    void *data = nullptr;
    vmaMapMemory(context->vmaAllocator, allocation, &data);
    memcpy(data, pData, dataLen);
    vmaUnmapMemory(context->vmaAllocator, allocation);
    
    VulkanBufferUtil::CopyBuffer(mContext->device, mContext->graphicsQueue, mContext->GetCommandPool(), stageBuffer, mBuffer, dataLen);
    vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);

    mBufferLength = (uint32_t)dataLen;
    mIndexType = indexType;
}

VKIndexBuffer::~VKIndexBuffer()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }

    if (mBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(mContext->vmaAllocator, mBuffer, mAllocation);
        mBuffer = VK_NULL_HANDLE;
    }
}

NAMESPACE_RENDERCORE_END
