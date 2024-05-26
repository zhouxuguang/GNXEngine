//
//  VKComputeBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputeBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKComputeBuffer::VKComputeBuffer(VulkanContextPtr context, size_t len)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, len,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, memType, mBuffer, mAllocation, nullptr);

    mStorageMode = StorageModePrivate;
    mBufferLength = (uint32_t)len;
}

VKComputeBuffer::VKComputeBuffer(VulkanContextPtr context, const void* buffer, size_t size, StorageMode mode)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (mode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mode, size,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);
    
    if (mode == StorageModePrivate)
    {
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
                                          bufferUsage, memType, stageBuffer, allocation, nullptr);
        
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, allocation, &data);
        memcpy(data, buffer, size);
        vmaUnmapMemory(context->vmaAllocator, allocation);
        
        VulkanBufferUtil::CopyBuffer(mContext->device, mContext->graphicsQueue, mContext->commandPool, stageBuffer, mBuffer, size);
        vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
    }
    else
    {
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, mAllocation, &data);
        memcpy(data, buffer, size);
        vmaUnmapMemory(context->vmaAllocator, mAllocation);
    }
    
    mStorageMode = mode;
    mBufferLength = (uint32_t)size;
}

VKComputeBuffer::~VKComputeBuffer()
{
    vmaDestroyBuffer(mContext->vmaAllocator, mBuffer, mAllocation);
}

uint32_t VKComputeBuffer::getBufferLength() const
{
    return mBufferLength;
}

void* VKComputeBuffer::mapBufferData() const
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

void VKComputeBuffer::unmapBufferData(void* bufferData) const
{
    vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
}

bool VKComputeBuffer::isValid() const
{
    return mBuffer != VK_NULL_HANDLE;
}

NAMESPACE_RENDERCORE_END
