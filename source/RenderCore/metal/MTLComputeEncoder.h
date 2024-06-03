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

NAMESPACE_RENDERCORE_BEGIN

class MTLComputeEncoder : public ComputeEncoder
{
public:
    MTLComputeEncoder(id<MTLCommandBuffer> commandBuffer);
    
    ~MTLComputeEncoder()
    {
        @autoreleasepool 
        {
            mComputeEncoder = nil;
            mCommandBuffer = nil;
        }
    }
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline);
    
    virtual void SetBuffer(ComputeBufferPtr buffer, uint32_t index);
    
    virtual void SetTexture(Texture2DPtr texture, uint32_t index);
    
    virtual void SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    virtual void SetOutTexture(Texture2DPtr texture, uint32_t index);
    
    virtual void SetOutTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index);
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ);
    
    virtual void EndEncode();
    
private:
    id<MTLComputeCommandEncoder> mComputeEncoder = nil;
    id<MTLCommandBuffer> mCommandBuffer = nil;
    MTLSize mThreadPerGroups;
};

typedef std::shared_ptr<MTLComputeEncoder> MTLComputeEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_COMPUTE_ENCODER_INCLUDE_HHFH */
