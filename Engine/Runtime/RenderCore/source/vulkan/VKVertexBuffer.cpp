//
//  VKVertexBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKVertexBuffer.h"
#include "VulkanBufferUtil.h"
#include "Runtime/BaseLib/include/LogService.h"

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
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkMemoryPropertyFlags privateMemType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryPropertyFlags sharedMemType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    bool useShared = (mode == StorageModeShared);

    if (!useShared)
    {
        // 尝试在 device-local 内存创建主缓冲区
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, size,
                                          bufferUsage, privateMemType, mBuffer, mAllocation, nullptr);

        if (mBuffer == VK_NULL_HANDLE)
        {
            // device-local 内存不足，fallback 主缓冲区到 host-visible 内存
            LOG_WARN("Vertex buffer device-local allocation failed (size=%.1f MB), falling back to host-visible memory",
                     (float)size / (1024.0f * 1024.0f));
            useShared = true;
        }
    }

    if (useShared)
    {
        // 主缓冲区使用 host-visible 内存（CPU 和 GPU 都可访问）
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
                                          bufferUsage, sharedMemType, mBuffer, mAllocation, nullptr);

        // Shared 路径：直接映射并拷贝数据
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, mAllocation, &data);
        memcpy(data, buffer, size);
        vmaUnmapMemory(context->vmaAllocator, mAllocation);
    }
    else
    {
        // Private 路径：通过 staging buffer 上传数据到 device-local 内存
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation stageAllocation = VK_NULL_HANDLE;
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
                                          bufferUsage, sharedMemType, stageBuffer, stageAllocation, nullptr);

        if (stageBuffer != VK_NULL_HANDLE)
        {
            // staging buffer 分配成功，一次性拷贝
            void *data = nullptr;
            vmaMapMemory(context->vmaAllocator, stageAllocation, &data);
            memcpy(data, buffer, size);
            vmaUnmapMemory(context->vmaAllocator, stageAllocation);

            VulkanBufferUtil::CopyBuffer(*mContext, mContext->graphicsQueue, mContext->GetCommandPool(),
                                         stageBuffer, mBuffer, size);
            vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, stageAllocation);
        }
        else
        {
            // staging buffer 也分配失败，使用较小的 host-visible 缓冲区分块上传
            LOG_WARN("Vertex buffer staging allocation failed (size=%.1f MB), using chunked upload to device-local",
                     (float)size / (1024.0f * 1024.0f));

            const VkDeviceSize chunkSize = 64 * 1024 * 1024;  // 64 MB 分块
            VkBuffer chunkBuffer = VK_NULL_HANDLE;
            VmaAllocation chunkAllocation = VK_NULL_HANDLE;
            VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, chunkSize,
                                              bufferUsage, sharedMemType, chunkBuffer, chunkAllocation, nullptr);

            if (chunkBuffer != VK_NULL_HANDLE)
            {
                VkDeviceSize offset = 0;
                while (offset < size)
                {
                    VkDeviceSize copySize = (size - offset > chunkSize) ? chunkSize : (size - offset);

                    void *data = nullptr;
                    vmaMapMemory(context->vmaAllocator, chunkAllocation, &data);
                    memcpy(data, (const uint8_t*)buffer + offset, (size_t)copySize);
                    vmaUnmapMemory(context->vmaAllocator, chunkAllocation);

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
            }
        }
    }
    
    mStorageMode = useShared ? StorageModeShared : StorageModePrivate;
    mBufferLength = (uint32_t)size;
}

VKVertexBuffer::~VKVertexBuffer()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }

    if (mBuffer != VK_NULL_HANDLE)
    {
        SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
        mBuffer = VK_NULL_HANDLE;
    }
}

uint32_t VKVertexBuffer::GetBufferLength() const
{
    return mBufferLength;
}

void* VKVertexBuffer::MapBufferData() const
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

void VKVertexBuffer::UnmapBufferData(void* bufferData) const
{
    vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
}

bool VKVertexBuffer::IsValid() const
{
    return mBuffer != VK_NULL_HANDLE;
}

VkBuffer VKVertexBuffer::GetGpuBuffer() const
{
    return mBuffer;
}

void VKVertexBuffer::SetName(const char* name)
{
    SetObjectName(mContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, name);
}

NAMESPACE_RENDERCORE_END
