//
//  VKRCBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Vulkan implementation of unified RCBuffer.
//

#include "VKRCBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKRCBuffer::VKRCBuffer(VulkanContextPtr context, const RCBufferDesc& desc)
    : mContext(context)
{
    CreateBuffer(desc, nullptr);
}

VKRCBuffer::VKRCBuffer(VulkanContextPtr context, const RCBufferDesc& desc, const void* data)
    : mContext(context)
{
    CreateBuffer(desc, data);
}

VKRCBuffer::~VKRCBuffer()
{
    if (mContext && mContext->device != VK_NULL_HANDLE && mBuffer != VK_NULL_HANDLE)
    {
        SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
        mBuffer = VK_NULL_HANDLE;
        mAllocation = VK_NULL_HANDLE;
    }
}

void VKRCBuffer::CreateBuffer(const RCBufferDesc& desc, const void* data)
{
    mSize = desc.size;
    mUsage = desc.usage;
    mStorageMode = desc.storageMode;
    
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (mStorageMode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    
    VkBufferUsageFlags vkUsage = ConvertToVkBufferUsage(mUsage);
    
    // Always add transfer flags for flexibility
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    if (data && mStorageMode == StorageModePrivate)
    {
        // Create staging buffer and copy data
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mStorageMode, mSize,
                                          vkUsage, memType, mBuffer, mAllocation, nullptr);
        
        // Create staging buffer
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingAllocation = VK_NULL_HANDLE;
        VkMemoryPropertyFlags stagingMemType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, mSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingMemType, 
                                          stageBuffer, stagingAllocation, nullptr);
        
        // Copy data to staging buffer
        void* stageData = nullptr;
        vmaMapMemory(mContext->vmaAllocator, stagingAllocation, &stageData);
        memcpy(stageData, data, mSize);
        vmaUnmapMemory(mContext->vmaAllocator, stagingAllocation);
        
        // Copy from staging to device buffer
        VulkanBufferUtil::CopyBuffer(*mContext, mContext->graphicsQueue, 
                                     mContext->GetCommandPool(), stageBuffer, mBuffer, mSize);
        
        // Destroy staging buffer
        vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, stagingAllocation);
    }
    else
    {
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mStorageMode, mSize,
                                          vkUsage, memType, mBuffer, mAllocation, nullptr);
        
        if (data)
        {
            // Shared mode - directly copy data
            void* bufferData = nullptr;
            vmaMapMemory(mContext->vmaAllocator, mAllocation, &bufferData);
            memcpy(bufferData, data, mSize);
            vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
        }
    }
}

VkBufferUsageFlags VKRCBuffer::ConvertToVkBufferUsage(RCBufferUsage usage) const
{
    VkBufferUsageFlags vkUsage = 0;
    
    if (HasUsage(usage, RCBufferUsage::VertexBuffer))
    {
        vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::IndexBuffer))
    {
        vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::UniformBuffer))
    {
        vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::StorageBuffer))
    {
        vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::IndirectBuffer))
    {
        vkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::TransferSrc))
    {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (HasUsage(usage, RCBufferUsage::TransferDst))
    {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    
    return vkUsage;
}

void* VKRCBuffer::Map() const
{
    if (mStorageMode != StorageModeShared)
    {
        return nullptr;
    }
    
    void* data = nullptr;
    vmaMapMemory(mContext->vmaAllocator, mAllocation, &data);
    return data;
}

void VKRCBuffer::Unmap() const
{
    if (mStorageMode == StorageModeShared)
    {
        vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
    }
}

void VKRCBuffer::SetName(const char* name)
{
    if (mContext && mContext->device && mBuffer != VK_NULL_HANDLE && name)
    {
        SetObjectName(mContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, name);
    }
}

NAMESPACE_RENDERCORE_END
