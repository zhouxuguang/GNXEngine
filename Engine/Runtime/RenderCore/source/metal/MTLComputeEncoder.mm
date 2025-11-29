//
//  MTLComputeEncoder.mm
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "MTLComputeEncoder.h"
#include "MTLComputePipeline.h"
#include "MTLComputeBuffer.h"
#include "MTLUniformBuffer.h"

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
        mMtlComputePipeline = mtlComputePipeline;
        assert(mtlComputePipeline);
        [mComputeEncoder setComputePipelineState:mtlComputePipeline->GetMTLComputePipelineState()];
        uint32_t x, y, z;
        mtlComputePipeline->GetThreadGroupSizes(x, y, z);
        mThreadPerGroups.width = x;
        mThreadPerGroups.height = y;
        mThreadPerGroups.depth = z;
    }
}

void MTLComputeEncoder::SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    @autoreleasepool
    {
        if (!mMtlComputePipeline)
        {
            assert(false);
            return;
        }
        
        NSUInteger realIndex = mMtlComputePipeline->GetResourceIndex(resourceName);
        if (realIndex == InvalidBindingIndex)
        {
            assert(false);
            return;
        }
        
        MTLUniformBuffer *mtlUniformBuffer = (MTLUniformBuffer*)buffer.get();
        assert(mtlUniformBuffer);
        if (mtlUniformBuffer->isBuffer())
        {
            [mComputeEncoder setBuffer:mtlUniformBuffer->getMTLBuffer() offset:0 atIndex:realIndex];
        }
        else
        {
            const std::vector<uint8_t>& data = mtlUniformBuffer->getBufferData();
            [mComputeEncoder setBytes:data.data() length:data.size() atIndex:realIndex];
        }
        
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

void MTLComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t index)
{
    @autoreleasepool 
    {
        MTLTextureBasePtr mtlTexture = std::dynamic_pointer_cast<MTLTextureBase>(texture);
        assert(mtlTexture);
        [mComputeEncoder setTexture:mtlTexture->getMTLTexture() atIndex:index];
    }
}

void MTLComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    @autoreleasepool
    {
        MTLTextureBasePtr mtlTexture = std::dynamic_pointer_cast<MTLTextureBase>(texture);
        assert(mtlTexture);
        [mComputeEncoder setTexture:mtlTexture->getMTLTexture() atIndex:index];
    }
}

void MTLComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t index)
{
    MTLComputeEncoder::SetTexture(texture, index);
}

void MTLComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
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
