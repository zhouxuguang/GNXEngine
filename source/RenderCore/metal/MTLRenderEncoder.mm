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
#include "MTLTexture2D.h"
#include "MTLTextureCube.h"
#include "MTLTextureSampler.h"
#include "MTLRenderTexture.h"
#include "MTLGraphicsPipeline.h"

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
void MTLRenderEncoder::setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
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
    
    id<MTLDepthStencilState> mtlDepthStencil = mtlGraphicsPipeline->getDepthStencilState();
    [mRenderEncoder setDepthStencilState:mtlDepthStencil];
}

/**
 
 @param buffer buffer对象
 @param index 绑定的索引
 */
void MTLRenderEncoder::setVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index)
{
    if (buffer == nullptr)
    {
        return;
    }
    id<MTLBuffer> mtlBuffer = std::dynamic_pointer_cast<MTLVertexBuffer>(buffer)->getMTLBuffer();
    [mRenderEncoder setVertexBuffer:mtlBuffer offset:offset atIndex:index];
}

/**
 设置顶点数据，以copy的方式直接设置，pData的大小dataLen最大为4096，即4K
 
 @param pData 数据指针
 @param dataLen 数据长度
 @param index 绑定的索引
 */
void MTLRenderEncoder::setVertexBytes(const void* pData, size_t dataLen, int index)
{
    assert(pData && dataLen <= 4096);
    if (!pData || dataLen <= 0)
    {
        return;
    }
    if (dataLen > 4096)
    {
        return;
    }
    
    [mRenderEncoder setVertexBytes:pData length:dataLen atIndex:index];
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void MTLRenderEncoder::setVertexUniformBuffer(UniformBufferPtr buffer, int index)
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
        [mRenderEncoder setVertexBuffer:mtlBuffer->getMTLBuffer() offset:0 atIndex:index];
    }
    else
    {
        const std::vector<uint8_t>& bufferData = mtlBuffer->getBufferData();
        [mRenderEncoder setVertexBytes:bufferData.data() length:bufferData.size() atIndex:index];
    }
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void MTLRenderEncoder::setFragmentUniformBuffer(UniformBufferPtr buffer, int index)
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
void MTLRenderEncoder::drawPrimitves(PrimitiveMode mode, int offset, int size)
{
    [mRenderEncoder drawPrimitives: ConvertPrimitiveType(mode)
                                                 vertexStart:offset vertexCount:size];
}

/**
 draw funton with index
 
 @param mode mode description
 @param size size description
 @param buffer buffer description
 @param offset offset description
 */
void MTLRenderEncoder::drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset)
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
    [mRenderEncoder drawIndexedPrimitives:(MTLPrimitiveType)mode indexCount : size
                                                         indexType : (MTLIndexType)type indexBuffer : mtlBuffer indexBufferOffset : byteOffset];
}

/**
 设置纹理和采样器

 @param texture 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void MTLRenderEncoder::setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index)
{
    if (!texture)
    {
        [mRenderEncoder setFragmentTexture:nil atIndex:index];
        return;
    }
    
    if (!sampler)
    {
        [mRenderEncoder setFragmentSamplerState:nil atIndex:index];
        return;
    }

    id<MTLTexture> mtlTexture = std::dynamic_pointer_cast<MTLTexture2D>(texture)->getMTLTexture();
    [mRenderEncoder setFragmentTexture:mtlTexture atIndex:index];
    id<MTLSamplerState> mtlSampler = std::dynamic_pointer_cast<MTLTextureSampler>(sampler)->getMTLSampler();
    [mRenderEncoder setFragmentSamplerState:mtlSampler atIndex:index];
}

/**
 设置立方体纹理和采样器

 @param textureCube 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void MTLRenderEncoder::setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index)
{
    if (!textureCube)
    {
        [mRenderEncoder setFragmentTexture:nil atIndex:index];
        return;
    }
    
    if (!sampler)
    {
        [mRenderEncoder setFragmentSamplerState:nil atIndex:index];
        return;
    }

    id<MTLTexture> mtlTexture = std::dynamic_pointer_cast<MTLTextureCube>(textureCube)->getMTLTexture();
    [mRenderEncoder setFragmentTexture:mtlTexture atIndex:index];
    id<MTLSamplerState> mtlSampler = std::dynamic_pointer_cast<MTLTextureSampler>(sampler)->getMTLSampler();
    [mRenderEncoder setFragmentSamplerState:mtlSampler atIndex:index];
}

/**
 设置渲染纹理和采样器

 @param renderTexture 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void MTLRenderEncoder::setFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index)
{    
    if (!renderTexture)
    {
        [mRenderEncoder setFragmentTexture:nil atIndex:index];
        return;
    }
    
    if (!sampler)
    {
        [mRenderEncoder setFragmentSamplerState:nil atIndex:index];
        return;
    }

    id<MTLTexture> mtlTexture = std::dynamic_pointer_cast<MTLRenderTexture>(renderTexture)->getMTLTexture();
    [mRenderEncoder setFragmentTexture:mtlTexture atIndex:index];
    id<MTLSamplerState> mtlSampler = std::dynamic_pointer_cast<MTLTextureSampler>(sampler)->getMTLSampler();
    [mRenderEncoder setFragmentSamplerState:mtlSampler atIndex:index];
}

NAMESPACE_RENDERCORE_END
