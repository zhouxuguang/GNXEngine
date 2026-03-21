//
//  MTLComputeEncoder.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_MTL_COMPUTE_ENCODER_INCLUDE_HHFH
#define GNX_ENGINE_MTL_COMPUTE_ENCODER_INCLUDE_HHFH

#include "MTLRenderDefine.h"
#include "ComputeEncoder.h"
#include "MTLComputePipeline.h"
#include "MTLTextureBase.h"
#include "MTLRCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLComputeEncoder : public ComputeEncoder
{
public:
    // 构造函数：支持并发计算类型
    MTLComputeEncoder(id<MTLCommandBuffer> commandBuffer, bool enableConcurrent = false);
    
    ~MTLComputeEncoder()
    {
        @autoreleasepool
        {
            mComputeEncoder = nil;
            mCommandBuffer = nil;
        }
    }
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline);
    
    virtual void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);
    
    // RCBuffer接口
    virtual void SetStorageBuffer(RCBufferPtr buffer, uint32_t index);
    
    // SetTexture - 通过索引
    virtual void SetTexture(RCTexturePtr texture, uint32_t index);
    
    virtual void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    // SetTexture - 通过资源名
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture);
    
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel);
    
    // SetOutTexture - 通过索引
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t index);
    
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    // SetOutTexture - 通过资源名
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture);
    
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel);
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ);
    
    virtual void EndEncode();
    
private:
    id<MTLComputeCommandEncoder> mComputeEncoder = nil;
    id<MTLCommandBuffer> mCommandBuffer = nil;
    MTLSize mThreadPerGroups;
    MTLComputePipeline *mMtlComputePipeline = nullptr;
    bool mEnableConcurrent = false;  // 是否启用并发计算
};

typedef std::shared_ptr<MTLComputeEncoder> MTLComputeEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMPUTE_ENCODER_INCLUDE_HHFH */
