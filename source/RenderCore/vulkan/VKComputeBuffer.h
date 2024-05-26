//
//  VKComputeBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_VK_COMPUTE_BUFFER_INCLVSDSFJMDKSJ
#define GNX_ENGINE_VK_COMPUTE_BUFFER_INCLVSDSFJMDKSJ

#include "VulkanContext.h"
#include "ComputeBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKComputeBuffer : public ComputeBuffer
{
public:
    VKComputeBuffer(VulkanContextPtr context, size_t len);
    
    VKComputeBuffer(VulkanContextPtr context, const void* buffer, size_t size, StorageMode mode);
    
    ~VKComputeBuffer();
    
    virtual uint32_t getBufferLength() const;
    
    virtual void* mapBufferData() const;
    
    virtual void unmapBufferData(void* bufferData) const;
    
    virtual bool isValid() const;
    
    VkBuffer GetBuffer() const
    {
        return mBuffer;
    }
    
private:
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    StorageMode mStorageMode;
    uint32_t mBufferLength = 0;

    VmaAllocation mAllocation = VK_NULL_HANDLE;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_COMPUTE_BUFFER_INCLVSDSFJMDKSJ */
