//
//  VKUniformBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKUniformBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKUniformBuffer::VKUniformBuffer(VulkanContextPtr context, uint32_t size)
{
    mContext = context;
    
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size, bufferUsage, memType, mBuffer, mAllocation, nullptr);
    
    mBufferLength = (uint32_t)size;
}

VKUniformBuffer::~VKUniformBuffer()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }
    
    // 这里先注释掉，因为模型的UBO目前是随时创建的，后续这个资源管理需要继续优化

//    if (mBuffer != VK_NULL_HANDLE)
//    {
//        vmaDestroyBuffer(mContext->vmaAllocator, mBuffer, mAllocation);
//        mBuffer = VK_NULL_HANDLE;
//    }
}

void VKUniformBuffer::setData(const void* data, uint32_t offset, uint32_t dataSize)
{
    uint8_t *pData = nullptr;
    vmaMapMemory(mContext->vmaAllocator, mAllocation, (void**)&pData);
    memcpy(pData + offset, data, dataSize);
    vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
}

NAMESPACE_RENDERCORE_END
