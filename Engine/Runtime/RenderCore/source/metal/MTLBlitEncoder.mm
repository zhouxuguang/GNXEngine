//
//  MTLBlitEncoder.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#include "MTLBlitEncoder.h"
#include "MTLVertexBuffer.h"
#include "MTLIndexBuffer.h"
#include "MTLRCBuffer.h"
#include "MTLTextureBase.h"

NAMESPACE_RENDERCORE_BEGIN

MTLBlitEncoder::MTLBlitEncoder(id<MTLCommandBuffer> commandBuffer)
{
    @autoreleasepool
    {
        mCommandBuffer = commandBuffer;
        mBlitEncoder = [commandBuffer blitCommandEncoder];
    }
}

MTLBlitEncoder::~MTLBlitEncoder()
{
    @autoreleasepool
    {
        if (mBlitEncoder)
        {
            [mBlitEncoder endEncoding];
            mBlitEncoder = nil;
        }
        mCommandBuffer = nil;
    }
}

id<MTLBuffer> MTLBlitEncoder::GetMTLBuffer(VertexBufferPtr buffer) const
{
    if (!buffer)
    {
        return nil;
    }
    
    // 尝试转换为MTLVertexBuffer
    MTLVertexBufferPtr mtlVertexBuffer = std::dynamic_pointer_cast<MTLVertexBuffer>(buffer);
    if (mtlVertexBuffer)
    {
        return mtlVertexBuffer->getMTLBuffer();
    }
    
    // 尝试转换为MTLIndexBuffer（也是Buffer）
    MTLIndexBufferPtr mtlIndexBuffer = std::dynamic_pointer_cast<MTLIndexBuffer>(buffer);
    if (mtlIndexBuffer)
    {
        return mtlIndexBuffer->getMTLBuffer();
    }
    
    return nil;
}

id<MTLBuffer> MTLBlitEncoder::GetMTLBufferFromRC(RCBufferPtr buffer) const
{
    if (!buffer)
    {
        return nil;
    }
    
    // 尝试转换为MTLRCBuffer
    MTLRCBufferPtr mtlRCBuffer = std::dynamic_pointer_cast<MTLRCBuffer>(buffer);
    if (mtlRCBuffer)
    {
        return mtlRCBuffer->GetMTLBuffer();
    }
    
    return nil;
}

id<MTLTexture> MTLBlitEncoder::GetMTLTexture(RCTexturePtr texture) const
{
    if (!texture)
    {
        return nil;
    }
    
    // 尝试转换为MTLTextureBase
    MTLTextureBasePtr mtlTexture = std::dynamic_pointer_cast<MTLTextureBase>(texture);
    if (mtlTexture)
    {
        return mtlTexture->getMTLTexture();
    }
    
    return nil;
}

// ==================== Buffer操作 ====================

void MTLBlitEncoder::CopyBufferToBuffer(VertexBufferPtr source,
                                         uint64_t sourceOffset,
                                         VertexBufferPtr destination,
                                         uint64_t destinationOffset,
                                         uint64_t size)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLBuffer> sourceBuffer = GetMTLBuffer(source);
    id<MTLBuffer> destBuffer = GetMTLBuffer(destination);
    
    if (sourceBuffer && destBuffer)
    {
        [mBlitEncoder copyFromBuffer:sourceBuffer
                        sourceOffset:sourceOffset
                            toBuffer:destBuffer
                   destinationOffset:destinationOffset
                                size:size];
    }
}

void MTLBlitEncoder::FillBuffer(VertexBufferPtr destination,
                               uint64_t destinationOffset,
                               const void* data,
                               uint64_t dataSize)
{
    if (!mBlitEncoder || !data || dataSize == 0)
    {
        return;
    }
    
    id<MTLBuffer> destBuffer = GetMTLBuffer(destination);
    if (destBuffer)
    {
        [mBlitEncoder fillBuffer:destBuffer
                         range:NSMakeRange(destinationOffset, dataSize)
                          value:*(const uint8_t*)data];
    }
}

// ==================== RCBuffer操作（新接口） ====================

void MTLBlitEncoder::CopyBuffer(RCBufferPtr source,
                                uint64_t sourceOffset,
                                RCBufferPtr destination,
                                uint64_t destinationOffset,
                                uint64_t size)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLBuffer> sourceBuffer = GetMTLBufferFromRC(source);
    id<MTLBuffer> destBuffer = GetMTLBufferFromRC(destination);
    
    if (sourceBuffer && destBuffer)
    {
        [mBlitEncoder copyFromBuffer:sourceBuffer
                        sourceOffset:sourceOffset
                            toBuffer:destBuffer
                   destinationOffset:destinationOffset
                                size:size];
    }
}

void MTLBlitEncoder::CopyTextureToBuffer(RCTexturePtr source,
                                         uint32_t sourceSlice,
                                         uint32_t sourceMipLevel,
                                         const Rect2D& sourceOffset,
                                         const Rect2D& sourceSize,
                                         RCBufferPtr destination,
                                         uint64_t destinationOffset,
                                         uint64_t destinationBytesPerRow,
                                         uint64_t destinationBytesPerImage)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLTexture> sourceTexture = GetMTLTexture(source);
    id<MTLBuffer> destBuffer = GetMTLBufferFromRC(destination);
    
    if (sourceTexture && destBuffer)
    {
        MTLOrigin origin = MTLOriginMake(sourceOffset.offsetX, sourceOffset.offsetY, 0);
        MTLSize size = MTLSizeMake(sourceSize.width, sourceSize.height, 1);
        
        [mBlitEncoder copyFromTexture:sourceTexture
                          sourceSlice:sourceSlice
                          sourceLevel:sourceMipLevel
                         sourceOrigin:origin
                           sourceSize:size
                             toBuffer:destBuffer
                    destinationOffset:destinationOffset
               destinationBytesPerRow:destinationBytesPerRow
             destinationBytesPerImage:destinationBytesPerImage];
    }
}

void MTLBlitEncoder::CopyBufferToTexture(RCBufferPtr source,
                                         uint64_t sourceOffset,
                                         uint64_t sourceBytesPerRow,
                                         uint64_t sourceBytesPerImage,
                                         RCTexturePtr destination,
                                         uint32_t destinationSlice,
                                         uint32_t destinationMipLevel,
                                         const Rect2D& destinationOffset,
                                         const Rect2D& destinationSize)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLBuffer> sourceBuffer = GetMTLBufferFromRC(source);
    id<MTLTexture> destTexture = GetMTLTexture(destination);
    
    if (sourceBuffer && destTexture)
    {
        MTLOrigin origin = MTLOriginMake(destinationOffset.offsetX, destinationOffset.offsetY, 0);
        MTLSize size = MTLSizeMake(destinationSize.width, destinationSize.height, 1);
        
        [mBlitEncoder copyFromBuffer:sourceBuffer
                        sourceOffset:sourceOffset
                   sourceBytesPerRow:sourceBytesPerRow
                 sourceBytesPerImage:sourceBytesPerImage
                         sourceSize:size
                          toTexture:destTexture
                   destinationSlice:destinationSlice
                   destinationLevel:destinationMipLevel
                  destinationOrigin:origin];
    }
}

// ==================== Texture到Buffer操作 ====================

void MTLBlitEncoder::CopyTextureToBuffer(RCTexturePtr source,
                                          uint32_t sourceSlice,
                                          uint32_t sourceMipLevel,
                                          const Rect2D& sourceOffset,
                                          const Rect2D& sourceSize,
                                          VertexBufferPtr destination,
                                          uint64_t destinationOffset,
                                          uint64_t destinationBytesPerRow,
                                          uint64_t destinationBytesPerImage)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLTexture> sourceTexture = GetMTLTexture(source);
    id<MTLBuffer> destBuffer = GetMTLBuffer(destination);
    
    if (sourceTexture && destBuffer)
    {
        MTLOrigin origin = MTLOriginMake(sourceOffset.offsetX, sourceOffset.offsetY, 0);
        MTLSize size = MTLSizeMake(sourceSize.width, sourceSize.height, 1);
        
        [mBlitEncoder copyFromTexture:sourceTexture
                          sourceSlice:sourceSlice
                          sourceLevel:sourceMipLevel
                         sourceOrigin:origin
                           sourceSize:size
                             toBuffer:destBuffer
                    destinationOffset:destinationOffset
               destinationBytesPerRow:destinationBytesPerRow
             destinationBytesPerImage:destinationBytesPerImage];
    }
}

// ==================== Buffer到Texture操作 ====================

void MTLBlitEncoder::CopyBufferToTexture(VertexBufferPtr source,
                                         uint64_t sourceOffset,
                                         uint64_t sourceBytesPerRow,
                                         uint64_t sourceBytesPerImage,
                                         RCTexturePtr destination,
                                         uint32_t destinationSlice,
                                         uint32_t destinationMipLevel,
                                         const Rect2D& destinationOffset,
                                         const Rect2D& destinationSize)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLBuffer> sourceBuffer = GetMTLBuffer(source);
    id<MTLTexture> destTexture = GetMTLTexture(destination);
    
    if (sourceBuffer && destTexture)
    {
        MTLOrigin origin = MTLOriginMake(destinationOffset.offsetX, destinationOffset.offsetY, 0);
        MTLSize size = MTLSizeMake(destinationSize.width, destinationSize.height, 1);
        
//        [mBlitEncoder copyFromBuffer:sourceBuffer
//                        sourceOffset:sourceOffset
//                   sourceBytesPerRow:sourceBytesPerRow
//                 sourceBytesPerImage:sourceBytesPerImage
//                          toTexture:destTexture
//                   destinationSlice:destinationSlice
//                   destinationLevel:destinationMipLevel
//                  destinationOrigin:origin
//                    destinationSize:size];
    }
}

// ==================== Texture到Texture操作 ====================

void MTLBlitEncoder::CopyTextureToTexture(RCTexturePtr source,
                                           uint32_t sourceSlice,
                                           uint32_t sourceMipLevel,
                                           const Rect2D& sourceOffset,
                                           const Rect2D& sourceSize,
                                           RCTexturePtr destination,
                                           uint32_t destinationSlice,
                                           uint32_t destinationMipLevel,
                                           const Rect2D& destinationOffset,
                                           const Rect2D& destinationSize)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLTexture> sourceTexture = GetMTLTexture(source);
    id<MTLTexture> destTexture = GetMTLTexture(destination);
    
    if (sourceTexture && destTexture)
    {
        MTLOrigin sourceOrigin = MTLOriginMake(sourceOffset.offsetX, sourceOffset.offsetY, 0);
        MTLSize sourceSize = MTLSizeMake(sourceSize.width, sourceSize.height, 1);
        
        MTLOrigin destOrigin = MTLOriginMake(destinationOffset.offsetX, destinationOffset.offsetY, 0);
        
        [mBlitEncoder copyFromTexture:sourceTexture
                          sourceSlice:sourceSlice
                          sourceLevel:sourceMipLevel
                         sourceOrigin:sourceOrigin
                           sourceSize:sourceSize
                             toTexture:destTexture
                      destinationSlice:destinationSlice
                      destinationLevel:destinationMipLevel
                     destinationOrigin:destOrigin];
    }
}

// ==================== Mipmap操作 ====================

void MTLBlitEncoder::GenerateMipmaps(RCTexturePtr texture, uint32_t slice)
{
    if (!mBlitEncoder)
    {
        return;
    }
    
    id<MTLTexture> mtlTexture = GetMTLTexture(texture);
    if (mtlTexture)
    {
        [mBlitEncoder generateMipmapsForTexture:mtlTexture];
    }
}

void MTLBlitEncoder::GenerateMipmapsForRange(RCTexturePtr texture,
                                              uint32_t slice,
                                              uint32_t baseMipLevel,
                                              uint32_t levelCount)
{
    // Metal的generateMipmapsForTexture是生成整个纹理链的mipmap
    // 如果需要精确控制范围，需要手动实现
    GenerateMipmaps(texture, slice);
}

// ==================== Barrier操作 ====================

void MTLBlitEncoder::MemoryBarrier()
{
    // Metal使用栅栏(barrier)来确保内存访问同步
    // 对于Blit操作，通常不需要显式的内存屏障
    // 但在某些情况下可能需要使用synchronizeResource
    if (!mBlitEncoder)
    {
        return;
    }
    
    // Metal的BlitCommandEncoder不需要显式的内存屏障
    // 资源依赖关系会自动处理
}

// ==================== 编码结束 ====================

void MTLBlitEncoder::EndEncode()
{
    if (mBlitEncoder)
    {
        [mBlitEncoder endEncoding];
        mBlitEncoder = nil;
    }
}

NAMESPACE_RENDERCORE_END
