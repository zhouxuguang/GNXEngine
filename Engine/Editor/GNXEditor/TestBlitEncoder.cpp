//
//  TestBlitEncoder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/10.
//  BlitEncoder使用示例
//

#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/CommandQueue.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/VertexBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include <cstdint>

NAMESPACE_RENDERCORE_BEGIN

// 示例数据
static const uint8_t gExampleTextureData[512 * 512 * 4] = {};

/**
 * @brief 示例1：Buffer到Buffer的拷贝
 */
void Example_CopyBufferToBuffer(RenderDevicePtr renderDevice)
{
    const uint32_t dataSize = 1024;
    uint8_t sourceData[dataSize] = {0};
    
    // 创建源和目标Buffer
    VertexBufferPtr sourceBuffer = renderDevice->CreateVertexBufferWithBytes(sourceData, dataSize, StorageModeShared);
    VertexBufferPtr destBuffer = renderDevice->CreateVertexBufferWithLength(dataSize);
    
    // 获取命令队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    
    // 创建命令缓冲区
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 执行Buffer拷贝
    blitEncoder->CopyBufferToBuffer(sourceBuffer, 0, destBuffer, 0, dataSize);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
}

/**
 * @brief 示例2：Buffer到Texture的拷贝（纹理上传）
 */
void Example_BufferToTexture(RenderDevicePtr renderDevice, const void* textureData,
                          uint32_t width, uint32_t height, uint32_t bytesPerRow)
{
    // 创建暂存Buffer（Shared模式，CPU可写）
    VertexBufferPtr stagingBuffer = renderDevice->CreateVertexBufferWithBytes(
        textureData, width * height * 4, StorageModeShared);
    
    // 创建目标Texture（Private模式，GPU专用）
    RCTexture2DPtr texture = renderDevice->CreateTexture2D(
        kTexFormatRGBA8,
        TextureUsage::TextureUsageShaderRead | TextureUsage::TextureUsageRenderTarget,
        width, height, 1);
    
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 上传纹理数据
    Rect2D offset(0, 0, width, height);
    blitEncoder->CopyBufferToTexture(
        stagingBuffer, 0, bytesPerRow, 0,
        texture, 0, 0, offset, offset);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
}

/**
 * @brief 示例3：Texture到Buffer的拷贝（纹理下载）
 */
void Example_TextureToBuffer(RenderDevicePtr renderDevice, RCTexture2DPtr texture,
                           uint32_t width, uint32_t height)
{
    // 创建目标Buffer
    VertexBufferPtr destBuffer = renderDevice->CreateVertexBufferWithLength(width * height * 4);
    
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 下载纹理数据
    Rect2D region(0, 0, width, height);
    blitEncoder->CopyTextureToBuffer(
        texture, 0, 0, region, region,
        destBuffer, 0, width * 4, 0);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
    
    // 读取数据
    void* data = destBuffer->MapBufferData();
    // 处理数据...
    destBuffer->UnmapBufferData(data);
}

/**
 * @brief 示例4：Texture到Texture的拷贝
 */
void Example_CopyTextureToTexture(RenderDevicePtr renderDevice, RCTexture2DPtr srcTexture,
                                 RCTexture2DPtr dstTexture, uint32_t width, uint32_t height)
{
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 执行纹理拷贝
    Rect2D region(0, 0, width, height);
    blitEncoder->CopyTextureToTexture(
        srcTexture, 0, 0, region, region,
        dstTexture, 0, 0, region, region);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
}

/**
 * @brief 示例5：生成Mipmap
 */
void Example_GenerateMipmaps(RenderDevicePtr renderDevice, RCTexture2DPtr texture)
{
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 生成Mipmap
    blitEncoder->GenerateMipmaps(texture, 0);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
}

/**
 * @brief 示例6：使用BlitEncoder进行异步资源加载
 */
void Example_AsyncResourceLoading(RenderDevicePtr renderDevice, const void* textureData,
                                uint32_t width, uint32_t height)
{
    // 创建暂存Buffer
    VertexBufferPtr stagingBuffer = renderDevice->CreateVertexBufferWithBytes(
        textureData, width * height * 4, StorageModeShared);
    
    // 创建目标Texture
    RCTexture2DPtr texture = renderDevice->CreateTexture2D(
        kTexFormatRGBA8,
        TextureUsage::TextureUsageShaderRead | TextureUsage::TextureUsageRenderTarget,
        width, height, 1);
    
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 上传纹理数据
    Rect2D region(0, 0, width, height);
    blitEncoder->CopyBufferToTexture(
        stagingBuffer, 0, width * 4, 0,
        texture, 0, 0, region, region);
    
    // 添加内存屏障
    blitEncoder->MemoryBarrier();
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交命令（不等待完成）
    commandBuffer->Submit();
    
    // 注意：在实际应用中，应该使用信号量或Fence来确保资源就绪后再使用
    // 这里只是演示异步提交
}

/**
 * @brief 示例7：多个Blit操作串联
 */
void Example_MultipleBlitOperations(RenderDevicePtr renderDevice)
{
    const uint32_t size1 = 1024;
    uint8_t data1[size1] = {0};
    
    // 获取传输队列
    CommandQueuePtr queue = renderDevice->GetCommandQueue(QueueType::Transfer, 0);
    CommandBufferPtr commandBuffer = queue->CreateCommandBuffer();
    
    // 创建BlitEncoder
    BlitEncoderPtr blitEncoder = commandBuffer->CreateBlitEncoder();
    
    // 操作1：Buffer到Buffer
    VertexBufferPtr buffer1 = renderDevice->CreateVertexBufferWithBytes(data1, size1, StorageModeShared);
    VertexBufferPtr buffer2 = renderDevice->CreateVertexBufferWithLength(size1);
    blitEncoder->CopyBufferToBuffer(buffer1, 0, buffer2, 0, size1);
    
    // 操作2：Buffer到Texture
    RCTexture2DPtr texture1 = renderDevice->CreateTexture2D(kTexFormatRGBA8, TextureUsage::TextureUsageShaderRead, 512, 512, 1);
    Rect2D region(0, 0, 512, 512);
    blitEncoder->CopyBufferToTexture(buffer2, 0, 512 * 4, 0, texture1, 0, 0, region, region);
    
    // 操作3：生成Mipmap
    blitEncoder->GenerateMipmaps(texture1);
    
    // 结束编码
    blitEncoder->EndEncode();
    
    // 提交所有操作
    commandBuffer->Submit();
    commandBuffer->WaitUntilCompleted();
}

NAMESPACE_RENDERCORE_END
