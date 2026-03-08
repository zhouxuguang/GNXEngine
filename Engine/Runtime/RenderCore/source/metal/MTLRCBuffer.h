//
//  MTLRCBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Metal implementation of unified RCBuffer.
//

#ifndef GNX_ENGINE_MTL_RC_BUFFER_INCLUDE_H
#define GNX_ENGINE_MTL_RC_BUFFER_INCLUDE_H

#include "MTLRenderDefine.h"
#include "RCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief Metal implementation of RCBuffer
 * 
 * Metal uses unified memory model where the same buffer can be
 * used for multiple purposes. This implementation wraps MTLBuffer.
 */
class MTLRCBuffer : public RCBuffer
{
public:
    /**
     * @brief Create buffer with specified size (uninitialized)
     */
    MTLRCBuffer(id<MTLDevice> device, const RCBufferDesc& desc);
    
    /**
     * @brief Create buffer with initial data
     */
    MTLRCBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, 
                const RCBufferDesc& desc, const void* data);
    
    virtual ~MTLRCBuffer();
    
    // RCBuffer interface implementation
    virtual uint32_t GetSize() const override;
    virtual RCBufferUsage GetUsage() const override { return mUsage; }
    virtual void* Map() const override;
    virtual void Unmap() const override;
    virtual bool IsValid() const override;
    virtual void SetName(const char* name) override;
    
    // Metal-specific methods
    id<MTLBuffer> GetMTLBuffer() const { return mBuffer; }
    
private:
    void CreateBuffer(id<MTLDevice> device, const RCBufferDesc& desc);
    void CreateBufferWithData(id<MTLDevice> device, id<MTLCommandQueue> commandQueue,
                              const RCBufferDesc& desc, const void* data);
    MTLStorageMode ConvertToMTLStorageMode(StorageMode mode) const;
    
    id<MTLBuffer> mBuffer = nil;
    uint32_t mSize = 0;
    RCBufferUsage mUsage = RCBufferUsage::Unknown;
    StorageMode mStorageMode = StorageModePrivate;
};

typedef std::shared_ptr<MTLRCBuffer> MTLRCBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RC_BUFFER_INCLUDE_H */
