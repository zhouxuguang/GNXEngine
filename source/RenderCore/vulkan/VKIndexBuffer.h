//
//  VKIndexBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#ifndef GNX_ENGINE_VK_INDEX_BUFFER_SDMDGJ
#define GNX_ENGINE_VK_INDEX_BUFFER_SDMDGJ

#include "VulkanContext.h"
#include "IndexBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKIndexBuffer : public IndexBuffer
{
public:
    VKIndexBuffer(VulkanContextPtr context, IndexType indexType, const void* pData, uint32_t dataLen);
    
    ~VKIndexBuffer();
    
    VkBuffer GetBuffer() const
    {
        return mBuffer;
    }
    
    IndexType getIndexType()
    {
        return mIndexType;
    }
    
private:
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    
    uint32_t mBufferLength = 0;
    IndexType mIndexType;
};

using VKIndexBufferPtr = std::shared_ptr<VKIndexBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_INDEX_BUFFER_SDMDGJ */
