//
//  BlitEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//

#ifndef GNX_ENGINE_BLIT_ENCODER_INCLUDE_H
#define GNX_ENGINE_BLIT_ENCODER_INCLUDE_H

#include "RenderDefine.h"
#include "VertexBuffer.h"
#include "ComputeBuffer.h"
#include "RCTexture.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief BlitEncoder抽象接口
 *
 * 用于资源之间的拷贝操作，包括：
 * - Buffer到Buffer的拷贝
 * - Texture到Texture的拷贝（支持区域拷贝）
 * - Buffer到Texture的拷贝（纹理数据上传）
 * - Texture到Buffer的拷贝（纹理数据下载）
 * - Mipmap生成
 */
class BlitEncoder
{
public:
    BlitEncoder()
    {
    }
    
    virtual ~BlitEncoder()
    {
    }
    
    // ==================== Buffer操作 ====================
    
    /**
     * @brief 拷贝Buffer数据
     * @param source 源Buffer
     * @param sourceOffset 源偏移量（字节）
     * @param destination 目标Buffer
     * @param destinationOffset 目标偏移量（字节）
     * @param size 拷贝大小（字节）
     */
    virtual void CopyBufferToBuffer(VertexBufferPtr source,
                                   uint64_t sourceOffset,
                                   VertexBufferPtr destination,
                                   uint64_t destinationOffset,
                                   uint64_t size) = 0;
    
    /**
     * @brief 填充Buffer数据
     * @param destination 目标Buffer
     * @param destinationOffset 目标偏移量（字节）
     * @param data 填充数据
     * @param dataSize 数据大小（字节）
     */
    virtual void FillBuffer(VertexBufferPtr destination,
                          uint64_t destinationOffset,
                          const void* data,
                          uint64_t dataSize) = 0;
    
    // ==================== Texture到Buffer操作 ====================
    
    /**
     * @brief 从Texture拷贝数据到Buffer
     * @param source 源纹理
     * @param sourceSlice 源纹理数组切片索引
     * @param sourceMipLevel 源纹理Mipmap级别
     * @param sourceOffset 源纹理内偏移
     * @param sourceSize 源纹理拷贝区域大小
     * @param destination 目标Buffer
     * @param destinationOffset 目标Buffer偏移量
     * @param destinationBytesPerRow 目标Buffer每行字节数
     * @param destinationBytesPerImage 目标Buffer每图像字节数（3D纹理使用）
     */
    virtual void CopyTextureToBuffer(RCTexturePtr source,
                                    uint32_t sourceSlice,
                                    uint32_t sourceMipLevel,
                                    const Rect2D& sourceOffset,
                                    const Rect2D& sourceSize,
                                    VertexBufferPtr destination,
                                    uint64_t destinationOffset,
                                    uint64_t destinationBytesPerRow,
                                    uint64_t destinationBytesPerImage = 0) = 0;
    
    // ==================== Buffer到Texture操作 ====================
    
    /**
     * @brief 从Buffer拷贝数据到Texture
     * @param source 源Buffer
     * @param sourceOffset 源Buffer偏移量
     * @param sourceBytesPerRow 源Buffer每行字节数
     * @param sourceBytesPerImage 源Buffer每图像字节数（3D纹理使用）
     * @param destination 目标纹理
     * @param destinationSlice 目标纹理数组切片索引
     * @param destinationMipLevel 目标纹理Mipmap级别
     * @param destinationOffset 目标纹理内偏移
     * @param destinationSize 目标纹理拷贝区域大小
     */
    virtual void CopyBufferToTexture(VertexBufferPtr source,
                                    uint64_t sourceOffset,
                                    uint64_t sourceBytesPerRow,
                                    uint64_t sourceBytesPerImage,
                                    RCTexturePtr destination,
                                    uint32_t destinationSlice,
                                    uint32_t destinationMipLevel,
                                    const Rect2D& destinationOffset,
                                    const Rect2D& destinationSize) = 0;
    
    // ==================== Texture到Texture操作 ====================
    
    /**
     * @brief 拷贝Texture数据
     * @param source 源纹理
     * @param sourceSlice 源纹理数组切片索引
     * @param sourceMipLevel 源纹理Mipmap级别
     * @param sourceOffset 源纹理内偏移
     * @param sourceSize 源纹理拷贝区域大小
     * @param destination 目标纹理
     * @param destinationSlice 目标纹理数组切片索引
     * @param destinationMipLevel 目标纹理Mipmap级别
     * @param destinationOffset 目标纹理内偏移
     * @param destinationSize 目标纹理拷贝区域大小
     */
    virtual void CopyTextureToTexture(RCTexturePtr source,
                                     uint32_t sourceSlice,
                                     uint32_t sourceMipLevel,
                                     const Rect2D& sourceOffset,
                                     const Rect2D& sourceSize,
                                     RCTexturePtr destination,
                                     uint32_t destinationSlice,
                                     uint32_t destinationMipLevel,
                                     const Rect2D& destinationOffset,
                                     const Rect2D& destinationSize) = 0;
    
    // ==================== Mipmap操作 ====================
    
    /**
     * @brief 生成Texture的Mipmap链
     * @param texture 目标纹理
     * @param slice 纹理数组切片索引
     */
    virtual void GenerateMipmaps(RCTexturePtr texture, uint32_t slice = 0) = 0;
    
    /**
     * @brief 生成Texture的Mipmap链（指定范围）
     * @param texture 目标纹理
     * @param slice 纹理数组切片索引
     * @param baseMipLevel 起始Mipmap级别
     * @param levelCount 生成的Mipmap级别数量
     */
    virtual void GenerateMipmapsForRange(RCTexturePtr texture,
                                         uint32_t slice,
                                         uint32_t baseMipLevel,
                                         uint32_t levelCount) = 0;
    
    // ==================== Barrier操作 ====================
    
    /**
     * @brief 添加内存屏障，确保之前的操作完成
     */
    virtual void MemoryBarrier() = 0;
    
    /**
     * @brief 结束Blit编码
     */
    virtual void EndEncode() = 0;
};

typedef std::shared_ptr<BlitEncoder> BlitEncoderPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_BLIT_ENCODER_INCLUDE_H */
