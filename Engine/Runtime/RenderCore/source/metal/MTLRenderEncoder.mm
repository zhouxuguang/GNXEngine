//
//  MTLRenderEncoder.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLRenderEncoder.h"
#include "MTLVertexBuffer.h"
#include "MTLIndexBuffer.h"
#include "MTLUniformBuffer.h"
#include "MTLTextureSampler.h"
#include "MTLGraphicsPipeline.h"
#include "MTLTextureBase.h"
#include "MTLComputeBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

MTLRenderEncoder::MTLRenderEncoder(id<MTLRenderCommandEncoder> renderEncoder, const FrameBufferFormat& frameBufferFormat)
{
    mRenderEncoder = renderEncoder;
    mFrameBufferFormat = frameBufferFormat;
}

MTLRenderEncoder::~MTLRenderEncoder()
{
    //
}

void MTLRenderEncoder::EndEncode()
{
    [mRenderEncoder endEncoding];
}

/**
 设置图形管线
 */
void MTLRenderEncoder::SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
{
    MTLGraphicsPipelinePtr mtlGraphicsPipeline = std::dynamic_pointer_cast<MTLGraphicsPipeline>(graphicsPipeline);
    if (nullptr == mtlGraphicsPipeline)
    {
        return;
    }
    mtlGraphicsPipeline->Generate(mFrameBufferFormat);
    mMtlGraphicsPipeline = mtlGraphicsPipeline;
    
    id<MTLRenderPipelineState> mtlPipeline = mtlGraphicsPipeline->getRenderPipelineState();
    [mRenderEncoder setRenderPipelineState:mtlPipeline];
    
    id<MTLDepthStencilState> mtlDepthStencil = mtlGraphicsPipeline->GetDepthStencilState();
    [mRenderEncoder setDepthStencilState:mtlDepthStencil];
}

/**
 
 @param buffer buffer对象
 @param index 绑定的索引
 */
void MTLRenderEncoder::SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index)
{
    if (buffer == nullptr)
    {
        return;
    }
    id<MTLBuffer> mtlBuffer = std::dynamic_pointer_cast<MTLVertexBuffer>(buffer)->getMTLBuffer();
    [mRenderEncoder setVertexBuffer:mtlBuffer offset:offset atIndex:index];
}

void MTLRenderEncoder::SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index)
{
    if (buffer == nullptr)
    {
        return;
    }
    MTLRCBufferPtr mtlBuffer = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
    if (!mtlBuffer)
    {
        return;
    }
    [mRenderEncoder setVertexBuffer:mtlBuffer->GetMTLBuffer() offset:offset atIndex:index];
}

void MTLRenderEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage)
{
    if (!buffer || !mMtlGraphicsPipeline)
    {
        return;
    }
    
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    MTLRCBufferPtr mtlBuffer = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
    if (!mtlBuffer)
    {
        return;
    }
    
    NSUInteger realIndex = InvalidBindingIndex;
    if (stage == ShaderStage_Vertex)
    {
        realIndex = shader->GetVertexResourceBindIndex(resourceName);
        if (realIndex != InvalidBindingIndex)
        {
            [mRenderEncoder setVertexBuffer:mtlBuffer->GetMTLBuffer() offset:0 atIndex:realIndex];
        }
    }
    else if (stage == ShaderStage_Fragment)
    {
        realIndex = shader->GetFragmentResourceBindIndex(resourceName);
        if (realIndex != InvalidBindingIndex)
        {
            [mRenderEncoder setFragmentBuffer:mtlBuffer->GetMTLBuffer() offset:0 atIndex:realIndex];
        }
    }
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void MTLRenderEncoder::SetVertexUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }
    
    if (!mMtlGraphicsPipeline)
    {
        return;
    }
    
    int realIndex = mMtlGraphicsPipeline->GetVertexUniformOffset() + index;
    
    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setVertexBuffer:mtlBuffer->getMTLBuffer() offset:0 atIndex:realIndex];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setVertexBytes:bufferData.data() length:bufferData.size() atIndex:realIndex];
    }
}

void MTLRenderEncoder::SetVertexUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer)
{
    if (buffer == nullptr)
    {
        return;
    }
    
    if (!mMtlGraphicsPipeline)
    {
        return;
    }
    
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    NSUInteger realIndex = shader->GetVertexResourceBindIndex(resourceName);
    if (realIndex == InvalidBindingIndex)
    {
        return;
    }
    
    id<MTLBuffer> mtlBuffer = std::dynamic_pointer_cast<MTLComputeBuffer>(buffer)->getMTLBuffer();
    [mRenderEncoder setVertexBuffer:mtlBuffer offset:0 atIndex:realIndex];
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void MTLRenderEncoder::SetFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }
    
    if (!mMtlGraphicsPipeline)
    {
        return;
    }
    
    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setFragmentBuffer: mtlBuffer->getMTLBuffer() offset:0 atIndex:index];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setFragmentBytes:bufferData.data() length:bufferData.size() atIndex:index];
    }
}

void MTLRenderEncoder::SetFragmentUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer)
{
    return;
}

void MTLRenderEncoder::SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture)
{
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    NSUInteger texIndex = shader->GetFragmentResourceBindIndex(resourceName);
    if (texIndex == InvalidBindingIndex)
    {
        return;
    }
    
    if (!texture)
    {
        [mRenderEncoder setFragmentTexture:nil atIndex:texIndex];
    }
    else
    {
        id<MTLTexture> mtlTexture = std::dynamic_pointer_cast<MTLTextureBase>(texture)->getMTLTexture();
        [mRenderEncoder setFragmentTexture:mtlTexture atIndex:texIndex];
    }
}

void MTLRenderEncoder::SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    if (!buffer)
    {
        return;
    }
    
    if (!mMtlGraphicsPipeline)
    {
        return;
    }
    
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    NSUInteger realIndex = shader->GetVertexResourceBindIndex(resourceName);
    if (realIndex == InvalidBindingIndex)
    {
        return;
    }
    
    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setVertexBuffer:mtlBuffer->getMTLBuffer() offset:0 atIndex:realIndex];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setVertexBytes:bufferData.data() length:bufferData.size() atIndex:realIndex];
    }
}

void MTLRenderEncoder::SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    if (!buffer)
    {
        return;
    }
    
    if (!mMtlGraphicsPipeline)
    {
        return;
    }
    
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    NSUInteger realIndex = shader->GetFragmentResourceBindIndex(resourceName);
    if (realIndex == InvalidBindingIndex)
    {
        return;
    }
    
    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setFragmentBuffer: mtlBuffer->getMTLBuffer() offset:0 atIndex:realIndex];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setFragmentBytes:bufferData.data() length:bufferData.size() atIndex:realIndex];
    }
}

static MTLPrimitiveType ConvertPrimitiveType(const PrimitiveMode mode)
{
    switch (mode)
    {
        case PrimitiveMode_POINTS:
            return MTLPrimitiveTypePoint;
            break;
            
        case PrimitiveMode_LINES:
            return MTLPrimitiveTypeLine;
            break;
            
        case PrimitiveMode_LINE_STRIP:
            return MTLPrimitiveTypeLineStrip;
            break;
            
        case PrimitiveMode_TRIANGLES:
            return MTLPrimitiveTypeTriangle;
            break;
            
        case PrimitiveMode_TRIANGLE_STRIP:
            return MTLPrimitiveTypeTriangleStrip;
            break;
            
        default:
            break;
    }
    
    return MTLPrimitiveTypePoint;
}

/**
 draw function
 
 @param mode mode description
 @param offset offset description
 @param size size description
 */
void MTLRenderEncoder::DrawPrimitves(PrimitiveMode mode, int offset, int size)
{
    [mRenderEncoder drawPrimitives: ConvertPrimitiveType(mode)
                                                 vertexStart:offset vertexCount:size];
}

void MTLRenderEncoder::DrawInstancePrimitves(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount)
{
    [mRenderEncoder drawPrimitives: ConvertPrimitiveType(mode) vertexStart : offset vertexCount : size
                    instanceCount : instanceCount baseInstance : firstInstance];
}

/**
 draw funton with index
 
 @param mode mode description
 @param size size description
 @param buffer buffer description
 @param offset offset description
 */
void MTLRenderEncoder::DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset)
{
    if (!buffer)
    {
        return;
    }
    
    MTLIndexBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLIndexBuffer>(buffer);
    
    id<MTLBuffer> mtlBuffer = mtlBufferPtr->getMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }
    
    IndexType type = mtlBufferPtr->getIndexType();
    
    int byteOffset = offset * sizeof(uint16_t);
    if (type == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
    }
    
    //注意：indexBufferOffset是字节的偏移量
    [mRenderEncoder drawIndexedPrimitives:(MTLPrimitiveType)ConvertPrimitiveType(mode) indexCount : size
                                                         indexType : (MTLIndexType)type indexBuffer : mtlBuffer indexBufferOffset : byteOffset];
}

void MTLRenderEncoder::DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
                                           uint32_t firstInstance, uint32_t instanceCount)
{
    if (!buffer)
    {
        return;
    }
    
    MTLIndexBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLIndexBuffer>(buffer);
    
    id<MTLBuffer> mtlBuffer = mtlBufferPtr->getMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }
    
    IndexType type = mtlBufferPtr->getIndexType();
    
    int byteOffset = offset * sizeof(uint16_t);
    if (type == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
    }
    
    //注意：indexBufferOffset是字节的偏移量
    [mRenderEncoder drawIndexedPrimitives:(MTLPrimitiveType)ConvertPrimitiveType(mode) indexCount : size
                               indexType : (MTLIndexType)type indexBuffer : mtlBuffer indexBufferOffset : byteOffset
                            instanceCount: instanceCount baseVertex : 0 baseInstance : firstInstance];
}

void MTLRenderEncoder::DrawPrimitvesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride)
{
    if (!buffer)
    {
        return;
    }

    MTLComputeBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLComputeBuffer>(buffer);

    id<MTLBuffer> mtlBuffer = mtlBufferPtr->getMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }

    uint32_t currentOffset = offset;
    for (uint32_t i = 0; i < drawCount; i++)
    {
        [mRenderEncoder drawPrimitives:ConvertPrimitiveType(mode) indirectBuffer:mtlBuffer indirectBufferOffset:currentOffset];
        currentOffset += stride;
    }
}

void MTLRenderEncoder::DrawIndexedPrimitivesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride)
{
    if (!buffer)
    {
        return;
    }

    MTLComputeBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLComputeBuffer>(buffer);

    id<MTLBuffer> mtlBuffer = mtlBufferPtr->getMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }
    
    // 还未实现
}

// RCBuffer版本的间接绘制
void MTLRenderEncoder::DrawPrimitvesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride)
{
    if (!buffer)
    {
        return;
    }

    MTLRCBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
    if (!mtlBufferPtr)
    {
        return;
    }

    id<MTLBuffer> mtlBuffer = mtlBufferPtr->GetMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }

    uint32_t currentOffset = offset;
    for (uint32_t i = 0; i < drawCount; i++)
    {
        [mRenderEncoder drawPrimitives:ConvertPrimitiveType(mode) indirectBuffer:mtlBuffer indirectBufferOffset:currentOffset];
        currentOffset += stride;
    }
}

void MTLRenderEncoder::DrawIndexedPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride)
{
    if (!buffer)
    {
        return;
    }

    MTLRCBufferPtr mtlBufferPtr = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
    if (!mtlBufferPtr)
    {
        return;
    }

    id<MTLBuffer> mtlBuffer = mtlBufferPtr->GetMTLBuffer();
    if (!mtlBuffer)
    {
        return;
    }
    
    // 还未实现
}

void MTLRenderEncoder::SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler)
{
    MTLGraphicsShaderPtr shader = mMtlGraphicsPipeline->GetShader();
    if (!shader)
    {
        return;
    }
    
    NSUInteger texIndex = shader->GetFragmentResourceBindIndex(resourceName);
    if (texIndex == InvalidBindingIndex)
    {
        return;
    }
    NSUInteger samIndex = shader->GetFragmentResourceBindIndex(resourceName + "Sam");
    if (samIndex == InvalidBindingIndex)
    {
        return;
    }
    
    if (!texture)
    {
        [mRenderEncoder setFragmentTexture:nil atIndex:texIndex];
    }
    else
    {
        id<MTLTexture> mtlTexture = std::dynamic_pointer_cast<MTLTextureBase>(texture)->getMTLTexture();
        [mRenderEncoder setFragmentTexture:mtlTexture atIndex:texIndex];
    }
    
    if (!sampler)
    {
        [mRenderEncoder setFragmentSamplerState:nil atIndex:samIndex];
    }
    else
    {
        id<MTLSamplerState> mtlSampler = std::dynamic_pointer_cast<MTLTextureSampler>(sampler)->getMTLSampler();
        [mRenderEncoder setFragmentSamplerState:mtlSampler atIndex:samIndex];
    }
}

NAMESPACE_RENDERCORE_END
