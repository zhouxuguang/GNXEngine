//
//  MTLComputeEncoder.mm
//  rendercore
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "MTLComputeEncoder.h"
#include "MTLComputePipeline.h"
#include "MTLUniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLComputeEncoder::MTLComputeEncoder(id<MTLCommandBuffer> commandBuffer, bool enableConcurrent)
    : ComputeEncoder(), mEnableConcurrent(enableConcurrent)
{
    @autoreleasepool
    {
        assert(commandBuffer != nil);

        // Start a compute pass with optional concurrent dispatch type
        if (mEnableConcurrent)
        {
            // 检查是否支持并发调度类型（iOS 10+, macOS 10.13+）
            if (@available(iOS 10.0, macOS 10.13, *))
            {
                // 使用并发调度类型，允许计算和图形命令并发执行
                mComputeEncoder = [commandBuffer computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
            }
            else
            {
                // 不支持并发，回退到默认方式
                mComputeEncoder = [commandBuffer computeCommandEncoder];
            }
        }
        else
        {
            // 默认方式（串行执行）
            mComputeEncoder = [commandBuffer computeCommandEncoder];
        }

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

void MTLComputeEncoder::SetStorageBuffer(RCBufferPtr buffer, uint32_t index)
{
    @autoreleasepool
    {
        MTLRCBufferPtr mtlBuffer = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
        assert(mtlBuffer);
        [mComputeEncoder setBuffer:mtlBuffer->GetMTLBuffer() offset:0 atIndex:index];
    }
}

void MTLComputeEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer)
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
        
        MTLRCBufferPtr mtlBuffer = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
        assert(mtlBuffer);
        [mComputeEncoder setBuffer:mtlBuffer->GetMTLBuffer() offset:0 atIndex:realIndex];
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
        [mComputeEncoder setTexture:mtlTexture->getMTLTextureView(mipLevel, 0) atIndex:index];
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

void MTLComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture)
{
    @autoreleasepool
    {
        if (!mMtlComputePipeline || !texture)
        {
            return;
        }
        
        NSUInteger bindIndex = mMtlComputePipeline->GetResourceIndex(resourceName);
        if (bindIndex == InvalidBindingIndex)
        {
            assert(false);
            return;
        }
        
        SetTexture(texture, bindIndex);
    }
}

void MTLComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel)
{
    @autoreleasepool
    {
        if (!mMtlComputePipeline || !texture)
        {
            return;
        }
        
        NSUInteger bindIndex = mMtlComputePipeline->GetResourceIndex(resourceName);
        if (bindIndex == InvalidBindingIndex)
        {
            assert(false);
            return;
        }
        
        SetTexture(texture, mipLevel, bindIndex);
    }
}

void MTLComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture)
{
    @autoreleasepool
    {
        if (!mMtlComputePipeline || !texture)
        {
            return;
        }
        
        NSUInteger bindIndex = mMtlComputePipeline->GetResourceIndex(resourceName);
        if (bindIndex == InvalidBindingIndex)
        {
            assert(false);
            return;
        }
        
        SetOutTexture(texture, bindIndex);
    }
}

void MTLComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel)
{
    @autoreleasepool
    {
        if (!mMtlComputePipeline || !texture)
        {
            return;
        }
        
        NSUInteger bindIndex = mMtlComputePipeline->GetResourceIndex(resourceName);
        if (bindIndex == InvalidBindingIndex)
        {
            assert(false);
            return;
        }
        
        SetOutTexture(texture, mipLevel, bindIndex);
    }
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
