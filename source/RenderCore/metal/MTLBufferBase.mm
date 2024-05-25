//
//  MTLBufferBase.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLBufferBase.h"

NAMESPACE_RENDERCORE_BEGIN

void MTLBufferBase::createPrivateBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, id<MTLBuffer> sharedBuffer)
{
    @autoreleasepool
    {
        mBuffer = [device newBufferWithLength: [sharedBuffer length] options: MTLStorageModePrivate << MTLResourceStorageModeShift];
        id<MTLCommandBuffer> cmd_buffer = [commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
        [blit_encoder copyFromBuffer:sharedBuffer
                        sourceOffset:0
                            toBuffer:mBuffer
                   destinationOffset:0
                                size: [sharedBuffer length]];
        [blit_encoder endEncoding];
        [cmd_buffer commit];
        [cmd_buffer waitUntilCompleted];
    }
}

MTLBufferBase::MTLBufferBase(id<MTLDevice> device, size_t len, StorageMode type)
{
    @autoreleasepool
    {
        if (device)
        {
            if (type == StorageModeShared)
            {
                mBuffer = [device newBufferWithLength:len options:MTLStorageModeShared << MTLResourceStorageModeShift];
            }
            else
            {
                mBuffer = [device newBufferWithLength: len options: MTLStorageModePrivate << MTLResourceStorageModeShift];
            }
        }
    }
}

MTLBufferBase::MTLBufferBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const void* buffer, size_t size, StorageMode type)
{
    @autoreleasepool
    {
        if (device && buffer)
        {
            if (type == StorageModeShared)
            {
                mBuffer = [device newBufferWithBytes:buffer length:size options:MTLStorageModeShared << MTLResourceStorageModeShift];
            }
            else
            {
                id<MTLBuffer> bufferShared = [device newBufferWithBytes:buffer length:size options:MTLStorageModeShared << MTLResourceStorageModeShift];
                createPrivateBuffer(device, commandQueue, bufferShared);
            }
        }
    }
    
}

MTLBufferBase::~MTLBufferBase()
{
}

/**
 获取Buffer的长度
 
 @return uffer长度,单位btye
 */
size_t MTLBufferBase::getBufferLength() const
{
    if (mBuffer)
    {
        return [mBuffer length];
    }
    
    return 0;
}

/**
 获取buffer数据
 
 @return buffer数据起始地址
 */
void* MTLBufferBase::getBufferData() const
{
    if (mBuffer)
    {
        return [mBuffer contents];
    }
    return nullptr;
}

NAMESPACE_RENDERCORE_END
