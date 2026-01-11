//
//  MTLBlitEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#ifndef GNX_ENGINE_MTL_BLIT_ENCODER_INCLUDE_H
#define GNX_ENGINE_MTL_BLIT_ENCODER_INCLUDE_H

#include "MTLRenderDefine.h"
#include "BlitEncoder.h"
#include "MTLVertexBuffer.h"
#include "MTLTextureBase.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief Metal BlitEncoder实现
 *
 * 封装MTLBlitCommandEncoder，提供资源拷贝、Mipmap生成等功能
 */
class MTLBlitEncoder : public BlitEncoder
{
public:
    /**
     * @brief 构造函数
     * @param commandBuffer Metal命令缓冲区
     */
    MTLBlitEncoder(id<MTLCommandBuffer> commandBuffer);
    
    /**
     * @brief 析构函数
     */
    ~MTLBlitEncoder();
    
    // ==================== Buffer操作 ====================
    
    virtual void CopyBufferToBuffer(VertexBufferPtr source,
                                   uint64_t sourceOffset,
                                   VertexBufferPtr destination,
                                   uint64_t destinationOffset,
                                   uint64_t size) override;
    
    virtual void FillBuffer(VertexBufferPtr destination,
                          uint64_t destinationOffset,
                          const void* data,
                          uint64_t dataSize) override;
    
    // ==================== Texture到Buffer操作 ====================
    
    virtual void CopyTextureToBuffer(RCTexturePtr source,
                                    uint32_t sourceSlice,
                                    uint32_t sourceMipLevel,
                                    const Rect2D& sourceOffset,
                                    const Rect2D& sourceSize,
                                    VertexBufferPtr destination,
                                    uint64_t destinationOffset,
                                    uint64_t destinationBytesPerRow,
                                    uint64_t destinationBytesPerImage = 0) override;
    
    // ==================== Buffer到Texture操作 ====================
    
    virtual void CopyBufferToTexture(VertexBufferPtr source,
                                    uint64_t sourceOffset,
                                    uint64_t sourceBytesPerRow,
                                    uint64_t sourceBytesPerImage,
                                    RCTexturePtr destination,
                                    uint32_t destinationSlice,
                                    uint32_t destinationMipLevel,
                                    const Rect2D& destinationOffset,
                                    const Rect2D& destinationSize) override;
    
    // ==================== Texture到Texture操作 ====================
    
    virtual void CopyTextureToTexture(RCTexturePtr source,
                                     uint32_t sourceSlice,
                                     uint32_t sourceMipLevel,
                                     const Rect2D& sourceOffset,
                                     const Rect2D& sourceSize,
                                     RCTexturePtr destination,
                                     uint32_t destinationSlice,
                                     uint32_t destinationMipLevel,
                                     const Rect2D& destinationOffset,
                                     const Rect2D& destinationSize) override;
    
    // ==================== Mipmap操作 ====================
    
    virtual void GenerateMipmaps(RCTexturePtr texture, uint32_t slice = 0) override;
    
    virtual void GenerateMipmapsForRange(RCTexturePtr texture,
                                         uint32_t slice,
                                         uint32_t baseMipLevel,
                                         uint32_t levelCount) override;
    
    // ==================== Barrier操作 ====================
    
    virtual void MemoryBarrier() override;
    
    /**
     * @brief 结束Blit编码
     */
    virtual void EndEncode() override;
    
private:
    id<MTLBlitCommandEncoder> mBlitEncoder = nil;
    id<MTLCommandBuffer> mCommandBuffer = nil;
    
    /**
     * @brief 辅助函数：获取MTLBuffer
     */
    id<MTLBuffer> GetMTLBuffer(VertexBufferPtr buffer) const;
    
    /**
     * @brief 辅助函数：获取MTLTexture
     */
    id<MTLTexture> GetMTLTexture(RCTexturePtr texture) const;
};

typedef std::shared_ptr<MTLBlitEncoder> MTLBlitEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_BLIT_ENCODER_INCLUDE_H */
