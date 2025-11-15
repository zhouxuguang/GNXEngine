//
//  VKVertexBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#ifndef GNX_ENGINE_VK_VERTEXT_BUFFER_INCLUDE_DJKSJD
#define GNX_ENGINE_VK_VERTEXT_BUFFER_INCLUDE_DJKSJD

#include "VulkanContext.h"
#include "VertexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKVertexBuffer : public VertexBuffer
{
public:
    VKVertexBuffer(VulkanContextPtr context, size_t len, StorageMode mode);

    VKVertexBuffer(VulkanContextPtr context, const void* buffer, size_t size, StorageMode mode);

    ~VKVertexBuffer();

    virtual uint32_t GetBufferLength() const;
    
    virtual void* MapBufferData() const;

    virtual void UnmapBufferData(void* bufferData) const;
    
    virtual bool IsValid() const;

    VkBuffer GetGpuBuffer() const;
    
    /**
      设置名字
     */
    virtual void SetName(const char* name);
private:
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    StorageMode mStorageMode;
    uint32_t mBufferLength = 0;

    VmaAllocation mAllocation = VK_NULL_HANDLE;
};

using VKVertexBufferPtr = std::shared_ptr<VKVertexBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_VERTEXT_BUFFER_INCLUDE_DJKSJD */
