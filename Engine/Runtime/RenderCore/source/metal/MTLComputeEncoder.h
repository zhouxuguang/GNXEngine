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
    
    void SetComputePipeline(ComputePipelinePtr computePipeline) override;
    
    void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;
    
    // RCBuffer接口
    void SetStorageBuffer(RCBufferPtr buffer, uint32_t index) override;
    
    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer) override;
    
    // SetTexture - 通过索引
    void SetTexture(RCTexturePtr texture, uint32_t index) override;
    
    void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    
    // SetTexture - 通过资源名
    void SetTexture(const std::string& resourceName, RCTexturePtr texture) override;
    
    void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;
    
    // SetOutTexture - 通过索引
    void SetOutTexture(RCTexturePtr texture, uint32_t index) override;
    
    void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) override;
    
    // SetOutTexture - 通过资源名
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture) override;
    
    void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) override;
    
    void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) override;
    
    void EndEncode() override;
    
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
