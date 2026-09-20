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
 *
 * 内存策略（与旧的 VertexBuffer / IndexBuffer 实现保持一致，便于承载大网格数据）：
 *   - StorageModePrivate：优先 DEVICE_LOCAL；分配失败时回退到 host-visible，
 *     保证大索引/顶点缓冲区不会因为显存不足而创建失败。
 *   - 初始数据上传：host-visible 路径直接 memcpy；DEVICE_LOCAL 路径经 staging buffer，
 *     若 staging 也分配失败则按 64MB 分块上传。
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
    
    /// DEVICE_LOCAL 路径下的初始数据上传（staging / 分块 fallback）
    bool UploadToDeviceLocal(const void* data);
    
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
