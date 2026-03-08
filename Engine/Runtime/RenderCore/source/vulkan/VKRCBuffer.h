//
//  VKRCBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Vulkan implementation of unified RCBuffer.
//

#ifndef GNX_ENGINE_VK_RC_BUFFER_INCLUDE_H
#define GNX_ENGINE_VK_RC_BUFFER_INCLUDE_H

#include "VulkanContext.h"
#include "RCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief Vulkan implementation of RCBuffer
 * 
 * Maps RCBufferUsage to appropriate VkBufferUsageFlags.
 * Uses VMA for memory management.
 */
class VKRCBuffer : public RCBuffer
{
public:
    /**
     * @brief Create buffer with specified size (uninitialized)
     */
    VKRCBuffer(VulkanContextPtr context, const RCBufferDesc& desc);
    
    /**
     * @brief Create buffer with initial data
     */
    VKRCBuffer(VulkanContextPtr context, const RCBufferDesc& desc, const void* data);
    
    virtual ~VKRCBuffer();
    
    // RCBuffer interface implementation
    virtual uint32_t GetSize() const override { return mSize; }
    virtual RCBufferUsage GetUsage() const override { return mUsage; }
    virtual void* Map() const override;
    virtual void Unmap() const override;
    virtual bool IsValid() const override { return mBuffer != VK_NULL_HANDLE; }
    virtual void SetName(const char* name) override;
    
    // Vulkan-specific methods
    VkBuffer GetVkBuffer() const { return mBuffer; }
    
private:
    void CreateBuffer(const RCBufferDesc& desc, const void* data);
    
    VkBufferUsageFlags ConvertToVkBufferUsage(RCBufferUsage usage) const;
    
    VulkanContextPtr mContext = nullptr;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    uint32_t mSize = 0;
    RCBufferUsage mUsage = RCBufferUsage::Unknown;
    StorageMode mStorageMode = StorageModePrivate;
};

typedef std::shared_ptr<VKRCBuffer> VKRCBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RC_BUFFER_INCLUDE_H */
