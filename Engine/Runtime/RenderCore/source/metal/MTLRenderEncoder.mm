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
#include "MTLRCBuffer.h"

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
    
    if (mtlGraphicsPipeline->IsMeshPipeline())
    {
        // Mesh Pipeline 模式
        id<MTLRenderPipelineState> meshState = mtlGraphicsPipeline->getMeshPipelineState();
        if (meshState)
        {
            [mRenderEncoder setRenderPipelineState:meshState];
        }
    }
    else
    {
        // 传统 Graphics Pipeline 模式
        id<MTLRenderPipelineState> mtlPipeline = mtlGraphicsPipeline->getRenderPipelineState();
        [mRenderEncoder setRenderPipelineState:mtlPipeline];
    }
    
    // depth stencil state 两种模式共用
    id<MTLDepthStencilState> mtlDepthStencil = mtlGraphicsPipeline->GetDepthStencilState();
    [mRenderEncoder setDepthStencilState:mtlDepthStencil];
    
    // 设置填充模式（从 pipeline descriptor 中读取）
    FillMode fillMode = mtlGraphicsPipeline->GetDesc().fillMode;
    MTLTriangleFillMode mtlFillMode = (fillMode == FillModeWireframe) ? MTLTriangleFillModeLines : MTLTriangleFillModeFill;
    [mRenderEncoder setTriangleFillMode:mtlFillMode];
    
    // 设置默认的剔除模式（与 Vulkan VK_CULL_MODE_NONE 一致）和正面方向
    [mRenderEncoder setCullMode:MTLCullModeNone];
    [mRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
}

void MTLRenderEncoder::SetFillMode(FillMode fillMode)
{
    MTLTriangleFillMode mtlFillMode = (fillMode == FillModeWireframe) ? MTLTriangleFillModeLines : MTLTriangleFillModeFill;
    [mRenderEncoder setTriangleFillMode:mtlFillMode];
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

void MTLRenderEncoder::SetMeshUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }

    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setMeshBuffer:mtlBuffer->getMTLBuffer() offset:0 atIndex:index];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setMeshBytes:bufferData.data() length:bufferData.size() atIndex:index];
    }
}

void MTLRenderEncoder::SetObjectUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }

    MTLUniformBuffer *mtlBuffer = (MTLUniformBuffer *)buffer.get();
    if (mtlBuffer->isBuffer())
    {
        [mRenderEncoder setObjectBuffer:mtlBuffer->getMTLBuffer() offset:0 atIndex:index];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setObjectBytes:bufferData.data() length:bufferData.size() atIndex:index];
    }
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
void MTLRenderEncoder::DrawPrimitives(PrimitiveMode mode, int offset, int size)
{
    [mRenderEncoder drawPrimitives: ConvertPrimitiveType(mode)
                                                 vertexStart:offset vertexCount:size];
}

void MTLRenderEncoder::DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount)
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
void MTLRenderEncoder::DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex)
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
    MTLIndexType mtlIndexType = (MTLIndexType)type;
    
    int byteOffset = offset * sizeof(uint16_t);
    if (type == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
    }
    
    //注意：indexBufferOffset是字节的偏移量
    [mRenderEncoder drawIndexedPrimitives:(MTLPrimitiveType)ConvertPrimitiveType(mode) indexCount : size
                                                         indexType : mtlIndexType indexBuffer : mtlBuffer indexBufferOffset : byteOffset
                                                      instanceCount : 1 baseVertex : baseVertex baseInstance : 0];
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
    MTLIndexType mtlIndexType = (MTLIndexType)type;
    
    int byteOffset = offset * sizeof(uint16_t);
    if (type == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
    }
    
    //注意：indexBufferOffset是字节的偏移量
    [mRenderEncoder drawIndexedPrimitives:(MTLPrimitiveType)ConvertPrimitiveType(mode) indexCount : size
                               indexType : mtlIndexType indexBuffer : mtlBuffer indexBufferOffset : byteOffset
                            instanceCount: instanceCount baseVertex : 0 baseInstance : firstInstance];
}

// RCBuffer版本的间接绘制
void MTLRenderEncoder::DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
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

void MTLRenderEncoder::DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
    int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
    uint32_t drawCount, uint32_t stride)
{
    if (!indexBuffer || !indirectBuffer)
    {
        return;
    }

    MTLIndexBufferPtr mtlIndexBufferPtr = std::dynamic_pointer_cast<MTLIndexBuffer>(indexBuffer);
    if (!mtlIndexBufferPtr)
    {
        return;
    }

    id<MTLBuffer> mtlIdxBuffer = mtlIndexBufferPtr->getMTLBuffer();
    if (!mtlIdxBuffer)
    {
        return;
    }

    MTLRCBufferPtr mtlIndirectBufferPtr = std::dynamic_pointer_cast<MTLRCBuffer>(indirectBuffer);
    if (!mtlIndirectBufferPtr)
    {
        return;
    }

    id<MTLBuffer> mtlIndirectBuf = mtlIndirectBufferPtr->GetMTLBuffer();
    if (!mtlIndirectBuf)
    {
        return;
    }

    IndexType type = mtlIndexBufferPtr->getIndexType();
    MTLIndexType mtlIndexType = (MTLIndexType)type;

    int byteOffset = indexBufferOffset * sizeof(uint16_t);
    if (type == IndexType_UInt)
    {
        byteOffset = indexBufferOffset * sizeof(uint32_t);
    }

    uint32_t currentOffset = indirectBufferOffset;
    for (uint32_t i = 0; i < drawCount; i++)
    {
        [mRenderEncoder drawIndexedPrimitives:ConvertPrimitiveType(mode)
                                   indexType:mtlIndexType
                                 indexBuffer:mtlIdxBuffer
                           indexBufferOffset:byteOffset
                              indirectBuffer:mtlIndirectBuf
                        indirectBufferOffset:currentOffset];
        currentOffset += stride;
    }
}

// ===== Mesh Shader 绘制实现 =====

void MTLRenderEncoder::DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    if (!mMtlGraphicsPipeline || !mMtlGraphicsPipeline->IsMeshPipeline())
    {
        return;
    }
    
    MTLSize threadgroupsPerGrid = MTLSizeMake(groupCountX, groupCountY, groupCountZ);
    
    // threadsPerObjectThreadgroup 从 pipeline descriptor 中获取
    // Metal mesh shader 的线程组大小由 pipeline 创建时确定
    MTLGraphicsPipeline* pipeline = mMtlGraphicsPipeline.get();
    uint32_t meshThreadsX = pipeline->GetDesc().meshThreadgroupSizeX;
    uint32_t meshThreadsY = pipeline->GetDesc().meshThreadgroupSizeY;
    uint32_t meshThreadsZ = pipeline->GetDesc().meshThreadgroupSizeZ;
    MTLSize threadsPerMeshThreadgroup = MTLSizeMake(
        meshThreadsX > 0 ? meshThreadsX : 1,
        meshThreadsY > 0 ? meshThreadsY : 1,
        meshThreadsZ > 0 ? meshThreadsZ : 1
    );

    // Object Shader (Task Shader) threadgroup size
    // 如果没有 object shader，threadsPerObjectThreadgroup 会被忽略
    MTLSize threadsPerObjectThreadgroup = MTLSizeMake(1, 1, 1);

    if (@available(macOS 13.0, iOS 16.0, *))
    {
        [mRenderEncoder drawMeshThreadgroups:threadgroupsPerGrid
                 threadsPerObjectThreadgroup:threadsPerObjectThreadgroup
                   threadsPerMeshThreadgroup:threadsPerMeshThreadgroup];
    }
}

void MTLRenderEncoder::DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                              uint32_t drawCount, uint32_t stride)
{
    if (!buffer || !mMtlGraphicsPipeline || !mMtlGraphicsPipeline->IsMeshPipeline())
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
    
    MTLGraphicsPipeline* pipeline = mMtlGraphicsPipeline.get();
    uint32_t meshThreadsX = pipeline->GetDesc().meshThreadgroupSizeX;
    uint32_t meshThreadsY = pipeline->GetDesc().meshThreadgroupSizeY;
    uint32_t meshThreadsZ = pipeline->GetDesc().meshThreadgroupSizeZ;
    MTLSize threadsPerMeshThreadgroup = MTLSizeMake(
        meshThreadsX > 0 ? meshThreadsX : 1,
        meshThreadsY > 0 ? meshThreadsY : 1,
        meshThreadsZ > 0 ? meshThreadsZ : 1
    );

    // Object Shader (Task Shader) threadgroup size
    MTLSize threadsPerObjectThreadgroup = MTLSizeMake(1, 1, 1);
    
    // 注意：Metal 的 MTLDrawMeshThreadgroupsIndirectArguments 结构体与 Vulkan 的 DrawMeshTasksIndirectCommand 不同
    // Metal 结构体: { uint32_t threadgroupsPerGrid[3]; }
    // Vulkan 结构体: { uint32_t groupCountX; uint32_t groupCountY; uint32_t groupCountZ; }
    // 实际上两者的内存布局完全一致，都是 3 个连续的 uint32_t
    
    uint32_t currentOffset = offset;
    for (uint32_t i = 0; i < drawCount; i++)
    {
        if (@available(macOS 13.0, iOS 16.0, *))
        {
            [mRenderEncoder drawMeshThreadgroupsWithIndirectBuffer:mtlBuffer
                                               indirectBufferOffset:currentOffset
                                        threadsPerObjectThreadgroup:threadsPerObjectThreadgroup
                                          threadsPerMeshThreadgroup:threadsPerMeshThreadgroup];
        }
        currentOffset += stride;
    }
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

// ===== 动态渲染状态实现 =====

void MTLRenderEncoder::SetScissorRect(int x, int y, uint32_t width, uint32_t height)
{
    MTLScissorRect scissorRect;
    scissorRect.x = x;
    scissorRect.y = y;
    scissorRect.width = width;
    scissorRect.height = height;
    [mRenderEncoder setScissorRect:scissorRect];
}

void MTLRenderEncoder::SetDepthBias(float bias, float slopeScale, float clamp)
{
    [mRenderEncoder setDepthBias:bias slopeScale:slopeScale clamp:clamp];
}

void MTLRenderEncoder::SetStencilReference(uint32_t frontRef, uint32_t backRef)
{
    [mRenderEncoder setStencilFrontReferenceValue:frontRef backReferenceValue:backRef];
}

NAMESPACE_RENDERCORE_END
