//
//  MTLUniformBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNXENGINE_ENGINE_MTL_UNIFORM_BUFFER_INCLUDE
#define GNXENGINE_ENGINE_MTL_UNIFORM_BUFFER_INCLUDE

#include "MTLBufferBase.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLUniformBuffer : public UniformBuffer
{
public:
    MTLUniformBuffer(id<MTLDevice> device, uint32_t size);
    
    ~MTLUniformBuffer();
    
    virtual void setData(const void* data, uint32_t offset, uint32_t dataSize);
    
    id<MTLBuffer> getMTLBuffer()
    {
        if (mBuffer)
        {
            return mBuffer->getMTLBuffer();
        }
        return nullptr;
    }
    
    const std::vector<uint8_t>& getBufferData() const
    {
        return mBufferData;
    }
    
    bool isBuffer() const
    {
        return mIsBuufer;
    }
    
private:
    MTLBufferBasePtr mBuffer = nullptr;     // 大于4096时使用
    std::vector<uint8_t> mBufferData;      // 小于4096时使用
    bool mIsBuufer = false;
};

typedef std::shared_ptr<MTLUniformBuffer> MTLUniformBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_ENGINE_MTL_UNIFORM_BUFFER_INCLUDE */
