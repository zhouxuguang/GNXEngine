//
//  VKIndexBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#include "VKIndexBuffer.h"
#include "VulkanBufferUtil.h"
#include "Runtime/BaseLib/include/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

VKIndexBuffer::VKIndexBuffer(VulkanContextPtr context, IndexType indexType, const void* pData, uint32_t dataLen)
    :IndexBuffer(indexType, pData, dataLen), mContext(context)
{
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkMemoryPropertyFlags privateMemType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryPropertyFlags sharedMemType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    bool useShared = false;

    // 尝试在 device-local 内存创建主缓冲区
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModePrivate, dataLen,
                                      bufferUsage, privateMemType, mBuffer, mAllocation, nullptr);

    if (mBuffer == VK_NULL_HANDLE)
    {
        // device-local 内存不足，fallback 主缓冲区到 host-visible 内存
        LOG_WARN("Index buffer device-local allocation failed (size=%.1f MB), falling back to host-visible memory",
                 (float)dataLen / (1024.0f * 1024.0f));
        useShared = true;
    }

    if (useShared)
    {
        // 主缓冲区使用 host-visible 内存（CPU 和 GPU 都可访问）
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, dataLen,
                                          bufferUsage, sharedMemType, mBuffer, mAllocation, nullptr);

        // Shared 路径：直接映射并拷贝数据
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, mAllocation, &data);
        memcpy(data, pData, dataLen);
        vmaUnmapMemory(context->vmaAllocator, mAllocation);
    }
    else
    {
        // Private 路径：通过 staging buffer 上传数据到 device-local 内存
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation stageAllocation = VK_NULL_HANDLE;
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, dataLen,
                                          bufferUsage, sharedMemType, stageBuffer, stageAllocation, nullptr);

        if (stageBuffer != VK_NULL_HANDLE)
        {
            // staging buffer 分配成功，一次性拷贝
            void *data = nullptr;
            vmaMapMemory(context->vmaAllocator, stageAllocation, &data);
            memcpy(data, pData, dataLen);
            vmaUnmapMemory(context->vmaAllocator, stageAllocation);

            VulkanBufferUtil::CopyBuffer(*mContext, mContext->graphicsQueue, mContext->GetCommandPool(),
                                         stageBuffer, mBuffer, dataLen);
            vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, stageAllocation);
        }
        else
        {
            // staging buffer 也分配失败，使用较小的 host-visible 缓冲区分块上传
            LOG_WARN("Index buffer staging allocation failed (size=%.1f MB), using chunked upload to device-local",
                     (float)dataLen / (1024.0f * 1024.0f));

            const VkDeviceSize chunkSize = 64 * 1024 * 1024;  // 64 MB 分块
            VkBuffer chunkBuffer = VK_NULL_HANDLE;
            VmaAllocation chunkAllocation = VK_NULL_HANDLE;
            VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, chunkSize,
                                              bufferUsage, sharedMemType, chunkBuffer, chunkAllocation, nullptr);

            if (chunkBuffer != VK_NULL_HANDLE)
            {
                VkDeviceSize offset = 0;
                while (offset < dataLen)
                {
                    VkDeviceSize copySize = (dataLen - offset > chunkSize) ? chunkSize : (dataLen - offset);

                    void *data = nullptr;
                    vmaMapMemory(context->vmaAllocator, chunkAllocation, &data);
                    memcpy(data, (const uint8_t*)pData + offset, (size_t)copySize);
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
        // 使用垃圾收集器延迟销毁
        SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
        mBuffer = VK_NULL_HANDLE;
    }
}

NAMESPACE_RENDERCORE_END
