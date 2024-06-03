//
//  MTLComputeEncoder.mm
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "MTLComputeEncoder.h"
#include "MTLComputePipeline.h"
#include "MTLComputeBuffer.h"
#include "MTLTexture2D.h"
#include "MTLRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

MTLComputeEncoder::MTLComputeEncoder(id<MTLCommandBuffer> commandBuffer) : ComputeEncoder()
{
    @autoreleasepool
    {
        assert(commandBuffer != nil);
        
        // Start a compute pass.
        mComputeEncoder = [commandBuffer computeCommandEncoder];
        assert(mComputeEncoder != nil);
    }
}

void MTLComputeEncoder::SetComputePipeline(ComputePipelinePtr computePipeline)
{
    @autoreleasepool
    {
        MTLComputePipeline *mtlComputePipeline = (MTLComputePipeline*)computePipeline.get();
        assert(mtlComputePipeline);
        [mComputeEncoder setComputePipelineState:mtlComputePipeline->GetMTLComputePipelineState()];
        uint32_t x, y, z;
        mtlComputePipeline->GetThreadGroupSizes(x, y, z);
        mThreadPerGroups.width = x;
        mThreadPerGroups.height = y;
        mThreadPerGroups.depth = z;
    }
}

void MTLComputeEncoder::SetBuffer(ComputeBufferPtr buffer, uint32_t index)
{
    @autoreleasepool
    {
        MTLComputeBuffer *mtlComputeBuffer = (MTLComputeBuffer*)buffer.get();
        assert(mtlComputeBuffer);
        [mComputeEncoder setBuffer:mtlComputeBuffer->getMTLBuffer() offset:0 atIndex:index];
    }
}

void MTLComputeEncoder::SetTexture(Texture2DPtr texture, uint32_t index)
{
    @autoreleasepool 
    {
        MTLTexture2D* mtlTexture = (MTLTexture2D*)texture.get();
        assert(mtlTexture);
        [mComputeEncoder setTexture:mtlTexture->getMTLTexture() atIndex:index];
    }
}

void MTLComputeEncoder::SetTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    @autoreleasepool
    {
        MTLRenderTexture* mtlTexture = (MTLRenderTexture*)texture.get();
        assert(mtlTexture);
        [mComputeEncoder setTexture:mtlTexture->getMTLTexture() atIndex:index];
    }
}

void MTLComputeEncoder::SetOutTexture(Texture2DPtr texture, uint32_t index)
{
    MTLComputeEncoder::SetTexture(texture, index);
}

void MTLComputeEncoder::SetOutTexture(RenderTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    MTLComputeEncoder::SetTexture(texture, mipLevel, index);
}

void MTLComputeEncoder::Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
{
    @autoreleasepool
    {
        MTLSize groupCount = MTLSizeMake(threadGroupsX, threadGroupsY, threadGroupsZ);
        [mComputeEncoder dispatchThreadgroups:groupCount threadsPerThreadgroup:mThreadPerGroups];
    }
}

void MTLComputeEncoder::EndEncode()
{
    @autoreleasepool
    {
        // End the compute pass.
        [mComputeEncoder endEncoding];
    }
}

NAMESPACE_RENDERCORE_END
