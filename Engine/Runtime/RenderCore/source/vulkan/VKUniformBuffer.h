//
//  VKUniformBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH
#define GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH

#include "VulkanContext.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKUniformBuffer : public UniformBuffer
{
public:
    VKUniformBuffer(VulkanContextPtr context, uint32_t size);
    
    ~VKUniformBuffer();
    
    void SetData(const void* data, uint32_t offset, uint32_t dataSize) override;
    
    VkBuffer GetBuffer() const
    {
        return mBuffer;
    }
    
    // 用于 push constant 路径：读取 CPU shadow copy
    const void* GetShadowData() const
    {
        return mShadowCopy.data();
    }
    
    uint32_t GetSize() const
    {
        return mBufferLength;
    }
    
private:
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    StorageMode mStorageMode;
    uint32_t mBufferLength = 0;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    
    // ≤256B 时保留 CPU 数据拷贝，避免从 GPU buffer 回读
    std::vector<uint8_t> mShadowCopy;
    
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH */
