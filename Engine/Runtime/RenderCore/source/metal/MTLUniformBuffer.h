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
#include <string>

NAMESPACE_RENDERCORE_BEGIN

class MTLUniformBuffer : public UniformBuffer
{
public:
    MTLUniformBuffer(id<MTLDevice> device, uint32_t size);
    
    ~MTLUniformBuffer();
    
    void SetData(const void* data, uint32_t offset, uint32_t dataSize) override;
    void SetName(const char* name) override;
    
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
        return mIsBuufer || mBuffer != nullptr;
    }
    
private:
    void UpdateNamedBuffer();
    id<MTLDevice> mDevice;
    std::string mDebugName;
    MTLBufferBasePtr mBuffer = nullptr;    // Large UBOs or named small snapshots.
    std::vector<uint8_t> mBufferData;      // CPU data for UBOs up to 4096 bytes.
    bool mIsBuufer = false;
};

typedef std::shared_ptr<MTLUniformBuffer> MTLUniformBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNXENGINE_ENGINE_MTL_UNIFORM_BUFFER_INCLUDE */
