#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHEXECUTECONTEXT_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHEXECUTECONTEXT_H

#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "../RSDefine.h"

NS_RENDERSYSTEM_BEGIN

/**
 * FrameGraph执行上下文
 * 提供RHI抽象的命令缓冲区，用于资源状态管理
 *
 * - 通过CommandBuffer的RHI抽象，不暴露特定API的细节
 * - Vulkan后端会自动处理资源layout转换
 * - 其他后端（Metal、D3D12）有各自的资源状态管理
 */
struct FrameGraphExecuteContext
{
    // RHI抽象的命令缓冲区
    // Vulkan后端会利用此对象自动插入layout转换barrier
    RenderCore::CommandBufferPtr commandBuffer;

    // 保留字段，用于未来扩展
    void* reserved = nullptr;
};

NS_RENDERSYSTEM_END

#endif
