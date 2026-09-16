//
//  VKUniformBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKUniformBuffer.h"
#include "VulkanBufferUtil.h"

#include <algorithm>

NAMESPACE_RENDERCORE_BEGIN

// 兜底的帧槽位数量：多数交换链为 2~3 张图，取 4 保证 slotCount >= imageCount。
static const uint32_t kMinFrameSlotCount = 4;

VKUniformBuffer::VKUniformBuffer(VulkanContextPtr context, uint32_t size, uint32_t slotCount)
{
    mContext = context;
    
    mBufferLength = (uint32_t)size;
    
    // 帧槽位之间必须满足 uniform buffer 的偏移对齐要求
    uint32_t alignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    if (alignment == 0)
    {
        alignment = 256;
    }
    mSlotStride = ((mBufferLength + alignment - 1) / alignment) * alignment;
    if (mSlotStride == 0)
    {
        mSlotStride = alignment;   // size 为 0 的兜底，避免创建长度为 0 的 VkBuffer
    }
    
    // CPU 影子副本（SetData 只写这里，绑定时才上传到帧槽位）
    mShadowCopy.assign(mBufferLength, 0);
    mDataVersion = 1;
    
    CreateSlots(std::max<uint32_t>(slotCount, kMinFrameSlotCount));
}

VKUniformBuffer::~VKUniformBuffer()
{
    if (VK_NULL_HANDLE == mContext->device)
    {
        return;
    }
    
    mMappedData = nullptr;
    
    if (mBuffer != VK_NULL_HANDLE)
    {
        SafeDestroyBuffer(*mContext, mBuffer, mAllocation);
        mBuffer = VK_NULL_HANDLE;
    }
    
    for (auto& retired : mRetiredBuffers)
    {
        if (retired.first != VK_NULL_HANDLE)
        {
            SafeDestroyBuffer(*mContext, retired.first, retired.second);
        }
    }
    mRetiredBuffers.clear();
}

void VKUniformBuffer::CreateSlots(uint32_t slotCount)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkDeviceSize totalSize = (VkDeviceSize)mSlotStride * slotCount;
    
    VmaAllocationInfo allocationInfo = {};
    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, totalSize, bufferUsage, memType,
                                      mBuffer, mAllocation, &allocationInfo);
    
    mSlotCount = slotCount;
    mSlotVersions.assign(slotCount, 0);   // 0 表示尚未上传过任何版本
    
    // StorageModeShared 的分配带 VMA_ALLOCATION_CREATE_MAPPED_BIT，直接拿常驻映射指针
    mMappedData = (uint8_t*)allocationInfo.pMappedData;
}

void VKUniformBuffer::EnsureSlotCount(uint32_t slotCount)
{
    if (mBuffer != VK_NULL_HANDLE && mSlotCount >= slotCount)
    {
        return;
    }
    
    // 交换链 image count 变大（切窗口/重建 swapchain）时才需要扩容
    uint32_t newSlotCount = std::max<uint32_t>(slotCount, mSlotCount > 0 ? mSlotCount * 2 : kMinFrameSlotCount);
    
    VkBuffer oldBuffer = mBuffer;
    VmaAllocation oldAllocation = mAllocation;
    
    mMappedData = nullptr;
    mBuffer = VK_NULL_HANDLE;
    mAllocation = VK_NULL_HANDLE;
    
    CreateSlots(newSlotCount);
    
    if (oldBuffer != VK_NULL_HANDLE)
    {
        // 旧 buffer 可能仍被在飞行的帧引用，不能立即销毁，挂到退休列表里延后释放
        mRetiredBuffers.push_back(std::make_pair(oldBuffer, oldAllocation));
    }
}

void VKUniformBuffer::UploadSlot(uint32_t slot)
{
    if (!mMappedData || mShadowCopy.empty())
    {
        return;
    }
    
    size_t uploadSize = std::min<size_t>(mShadowCopy.size(), mBufferLength);
    memcpy(mMappedData + (size_t)slot * mSlotStride, mShadowCopy.data(), uploadSize);
    mSlotVersions[slot] = mDataVersion;
}

VkDeviceSize VKUniformBuffer::PrepareForFrame(uint32_t frameIndex)
{
    if (mBuffer == VK_NULL_HANDLE)
    {
        return 0;
    }
    
    EnsureSlotCount(frameIndex + 1);
    
    uint32_t slot = frameIndex % mSlotCount;
    
    if (mSlotVersions[slot] != mDataVersion)
    {
        UploadSlot(slot);
    }
    
    return (VkDeviceSize)slot * mSlotStride;
}

void VKUniformBuffer::SetData(const void* data, uint32_t offset, uint32_t dataSize)
{
    size_t requiredSize = (size_t)offset + dataSize;
    if (mShadowCopy.size() < requiredSize)
    {
        mShadowCopy.resize(requiredSize, 0);
    }
    memcpy(mShadowCopy.data() + offset, data, dataSize);
    
    // 版本号变化 → 各帧槽位在下次绑定时重新上传
    ++mDataVersion;
}

NAMESPACE_RENDERCORE_END
