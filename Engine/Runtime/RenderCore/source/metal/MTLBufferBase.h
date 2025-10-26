//
//  MTLBufferBase.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTLBUFFER_BASE_INCLUDE_H
#define GNX_ENGINE_MTLBUFFER_BASE_INCLUDE_H

#include "MTLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLBufferBase
{
public:
    MTLBufferBase(id<MTLDevice> device, size_t len, StorageMode mode);
    
    MTLBufferBase(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, const void* buffer, size_t size, StorageMode mode);
    
    ~MTLBufferBase();
    
public:
    /**
     获取Buffer的长度
     
     @return uffer长度,单位btye
     */
    virtual size_t getBufferLength() const;
    
    /**
     获取buffer数据
     
     @return buffer数据起始地址
     */
    virtual void* getBufferData() const;
    
    id<MTLBuffer> getMTLBuffer(){return mBuffer;};
    
private:
    
    id<MTLBuffer> mBuffer;
    
    //根据sharedbuffer创建私有buffer
    void createPrivateBuffer(id<MTLDevice> device, id<MTLCommandQueue> commandQueue, id<MTLBuffer> sharedBuffer);
};

typedef std::unique_ptr<MTLBufferBase> MTLBufferBasePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTLBUFFER_BASE_INCLUDE_H */
