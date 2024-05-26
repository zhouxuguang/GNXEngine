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
    
    void setData(const void* data, uint32_t offset, uint32_t dataSize);
    
private:
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    StorageMode mStorageMode;
    uint32_t mBufferLength = 0;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH */
