//
//  VKRCBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Vulkan implementation of unified RCBuffer.
//

#include "VKRCBuffer.h"
#include "VulkanBufferUtil.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <algorithm>

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
    
    VkBufferUsageFlags vkUsage = ConvertToVkBufferUsage(mUsage);
    
    // Always add transfer flags for flexibility
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    const VkMemoryPropertyFlags deviceLocalMemType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkMemoryPropertyFlags hostVisibleMemType =
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    
    bool useShared = (mStorageMode == StorageModeShared);
    
    if (!useShared)
    {
        // 尝试在 device-local 内存创建主缓冲区
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, mSize,
                                          vkUsage, deviceLocalMemType, mBuffer, mAllocation, nullptr);
        
        if (mBuffer == VK_NULL_HANDLE)
        {
            // device-local 内存不足，fallback 主缓冲区到 host-visible 内存。
            // 大网格（如地形 master index buffer）在低显存设备上会走到这里，
            // 回退后 Map() 依然可用，行为与旧 VertexBuffer/IndexBuffer 一致。
            LOG_WARN("RCBuffer device-local allocation failed (size=%.1f MB, usage=0x%x), "
                     "falling back to host-visible memory",
                     (float)mSize / (1024.0f * 1024.0f), (uint32_t)mUsage);
            useShared = true;
        }
    }
    
    if (useShared)
    {
        // 主缓冲区使用 host-visible 内存（CPU 和 GPU 都可访问）
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, mSize,
                                          vkUsage, hostVisibleMemType, mBuffer, mAllocation, nullptr);
        
        if (mBuffer == VK_NULL_HANDLE || mAllocation == VK_NULL_HANDLE)
        {
            LOG_ERROR("RCBuffer host-visible allocation failed (size=%u)", mSize);
            return;
        }

        if (data)
        {
            void* mapped = nullptr;
            const VkResult mapResult = vmaMapMemory(mContext->vmaAllocator, mAllocation, &mapped);
            if (mapResult != VK_SUCCESS || mapped == nullptr)
            {
                LOG_ERROR("RCBuffer host-visible map failed (VkResult=%d)", (int)mapResult);
                SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
                mBuffer = VK_NULL_HANDLE;
                mAllocation = VK_NULL_HANDLE;
                return;
            }
            memcpy(mapped, data, mSize);
            vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
        }
        
        // 记录实际存储模式，保证 Map()/Unmap() 语义与实际内存一致
        mStorageMode = StorageModeShared;
        return;
    }
    
    if (data)
    {
        if (!UploadToDeviceLocal(data))
        {
            LOG_ERROR("RCBuffer initial upload failed (size=%u)", mSize);
            SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
            mBuffer = VK_NULL_HANDLE;
            mAllocation = VK_NULL_HANDLE;
        }
    }
}

bool VKRCBuffer::UploadToDeviceLocal(const void* data)
{
    // Private 路径：通过 staging buffer 上传数据到 device-local 内存
    VkBuffer stageBuffer = VK_NULL_HANDLE;
    VmaAllocation stageAllocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, mSize,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      stageBuffer, stageAllocation, nullptr);
    
    if (stageBuffer != VK_NULL_HANDLE)
    {
        // staging buffer 分配成功，一次性拷贝
        void* staged = nullptr;
        const VkResult mapResult = vmaMapMemory(mContext->vmaAllocator, stageAllocation, &staged);
        if (mapResult != VK_SUCCESS || staged == nullptr)
        {
            vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, stageAllocation);
            return false;
        }
        memcpy(staged, data, mSize);
        vmaUnmapMemory(mContext->vmaAllocator, stageAllocation);
        
        VulkanBufferUtil::CopyBuffer(*mContext, mContext->graphicsQueue, mContext->GetCommandPool(),
                                     stageBuffer, mBuffer, mSize);
        
        vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, stageAllocation);
        return true;
    }
    
    // staging buffer 也分配失败，使用较小的 host-visible 缓冲区分块上传
    LOG_WARN("RCBuffer staging allocation failed (size=%.1f MB), using chunked upload to device-local",
             (float)mSize / (1024.0f * 1024.0f));
    
    const VkDeviceSize chunkSize = std::min<VkDeviceSize>(mSize, 64ull * 1024ull * 1024ull);
    VkBuffer chunkBuffer = VK_NULL_HANDLE;
    VmaAllocation chunkAllocation = VK_NULL_HANDLE;
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, chunkSize,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      chunkBuffer, chunkAllocation, nullptr);
    
    if (chunkBuffer == VK_NULL_HANDLE)
    {
        return false;
    }
    
    const uint8_t* src = static_cast<const uint8_t*>(data);
    for (VkDeviceSize offset = 0; offset < mSize; )
    {
        const VkDeviceSize copySize = std::min<VkDeviceSize>(chunkSize, (VkDeviceSize)mSize - offset);
        
        void* chunkMapped = nullptr;
        const VkResult mapResult = vmaMapMemory(mContext->vmaAllocator, chunkAllocation, &chunkMapped);
        if (mapResult != VK_SUCCESS || chunkMapped == nullptr)
        {
            vmaDestroyBuffer(mContext->vmaAllocator, chunkBuffer, chunkAllocation);
            return false;
        }
        memcpy(chunkMapped, src + offset, (size_t)copySize);
        vmaUnmapMemory(mContext->vmaAllocator, chunkAllocation);
        
        // 分块拷贝到 device-local 主缓冲区的对应偏移位置
        VkCommandBuffer cmdBuffer = VulkanBufferUtil::BeginSingleTimeCommand(mContext->device, mContext->GetCommandPool());
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = copySize;
        vkCmdCopyBuffer(cmdBuffer, chunkBuffer, mBuffer, 1, &copyRegion);
        VulkanBufferUtil::EndSingleTimeCommand(*mContext, mContext->graphicsQueue, mContext->GetCommandPool(), cmdBuffer);
        
        offset += copySize;
    }
    
    vmaDestroyBuffer(mContext->vmaAllocator, chunkBuffer, chunkAllocation);
    return true;
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
    if (mAllocation == VK_NULL_HANDLE ||
        vmaMapMemory(mContext->vmaAllocator, mAllocation, &data) != VK_SUCCESS)
    {
        return nullptr;
    }
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
