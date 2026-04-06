//
//  MTLRCBuffer.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Metal implementation of unified RCBuffer.
//

#include "MTLRCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLRCBuffer::MTLRCBuffer(id<MTLDevice> device, const RCBufferDesc& desc)
{
    CreateBuffer(device, desc);
}

MTLRCBuffer::MTLRCBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue,
                         const RCBufferDesc& desc, const void* data)
{
    CreateBufferWithData(device, commandQueue, desc, data);
}

MTLRCBuffer::~MTLRCBuffer()
{
    // Metal uses ARC, buffer will be released automatically
    mBuffer = nil;
}

void MTLRCBuffer::CreateBuffer(id<MTLDevice> device, const RCBufferDesc& desc)
{
    mSize = desc.size;
    mUsage = desc.usage;
    mStorageMode = desc.storageMode;
    
    MTLStorageMode mtlStorageMode = ConvertToMTLStorageMode(desc.storageMode);
    MTLResourceOptions options = MTLResourceCPUCacheModeDefaultCache;
    
    if (mtlStorageMode == MTLStorageModeShared)
    {
        options |= MTLStorageModeShared;
    }
    else
    {
        options |= MTLStorageModePrivate;
    }
    
    @autoreleasepool
    {
        mBuffer = [device newBufferWithLength:desc.size options:options];
    }
}

void MTLRCBuffer::CreateBufferWithData(id<MTLDevice> device, id<MTLCommandQueue> commandQueue,
                                       const RCBufferDesc& desc, const void* data)
{
    mSize = desc.size;
    mUsage = desc.usage;
    mStorageMode = desc.storageMode;
    
    MTLStorageMode mtlStorageMode = ConvertToMTLStorageMode(desc.storageMode);
    
    @autoreleasepool
    {
        if (mtlStorageMode == MTLStorageModeShared)
        {
            // Shared memory - directly create with data
            MTLResourceOptions options = MTLResourceCPUCacheModeDefaultCache | MTLStorageModeShared;
            mBuffer = [device newBufferWithBytes:data length:desc.size options:options];
        }
        else
        {
            // Private memory - create staging buffer and copy
            // First create with shared storage
            MTLResourceOptions sharedOptions = MTLResourceCPUCacheModeDefaultCache | MTLStorageModeShared;
            id<MTLBuffer> sharedBuffer = [device newBufferWithBytes:data length:desc.size options:sharedOptions];
            
            // Create private buffer
            MTLResourceOptions privateOptions = MTLResourceCPUCacheModeDefaultCache | MTLStorageModePrivate;
            mBuffer = [device newBufferWithLength:desc.size options:privateOptions];
            
            // Copy using blit encoder
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            [blitEncoder copyFromBuffer:sharedBuffer sourceOffset:0
                                  toBuffer:mBuffer destinationOffset:0
                                      size:desc.size];
            [blitEncoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
        }
    }
}

MTLStorageMode MTLRCBuffer::ConvertToMTLStorageMode(StorageMode mode) const
{
    switch (mode)
    {
        case StorageModeShared:
            return MTLStorageModeShared;
        case StorageModePrivate:
            return MTLStorageModePrivate;
        default:
            return MTLStorageModePrivate;
    }
}

uint32_t MTLRCBuffer::GetSize() const
{
    if (mBuffer)
    {
        return (uint32_t)mBuffer.length;
    }
    return mSize;
}

void* MTLRCBuffer::Map() const
{
    if (mBuffer && mStorageMode == StorageModeShared)
    {
        return mBuffer.contents;
    }
    return nullptr;
}

void MTLRCBuffer::Unmap() const
{
    // Metal does not require explicit unmap for shared buffers
    // For managed buffers, we would call didModifyRange here
}

bool MTLRCBuffer::IsValid() const
{
    return mBuffer != nil;
}

void MTLRCBuffer::SetName(const char* name)
{
    if (mBuffer && name)
    {
        @autoreleasepool
        {
            mBuffer.label = [NSString stringWithUTF8String:name];
        }
    }
}

NAMESPACE_RENDERCORE_END
