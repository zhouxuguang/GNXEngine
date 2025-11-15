//
//  CommandBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#ifndef GNX_ENGINE_COMMAND_BUFFER_INCLUDE_GPPP_H
#define GNX_ENGINE_COMMAND_BUFFER_INCLUDE_GPPP_H

#include "RenderDefine.h"
#include "RenderEncoder.h"
#include "ComputeEncoder.h"
#include "RenderPass.h"

NAMESPACE_RENDERCORE_BEGIN

class CommandBuffer
{
public:
    CommandBuffer();
    
    ~CommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    virtual RenderEncoderPtr CreateDefaultRenderEncoder() const = 0;
    
    virtual RenderEncoderPtr CreateRenderEncoder(const RenderPass& renderPass) const = 0;
    
    virtual ComputeEncoderPtr CreateComputeEncoder() const = 0;
    
    //呈现到屏幕上，上屏
    virtual void PresentFrameBuffer() = 0;
    
    //等待命令缓冲区执行完成
    virtual void WaitUntilCompleted() = 0;
    
    // 开始调试标记
    virtual void BeginDebugGroup(const char* name, const float color[4]) = 0;
    
    // 结束调试标记
    virtual void EndDebugGroup() = 0;
};

typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;

class ScopedDebugMarker
{
public:
    ScopedDebugMarker(CommandBufferPtr commandBuffer, const char* name, const float color[4]) : mCommandBuffer(commandBuffer)
    {
        mCommandBuffer->BeginDebugGroup(name, color);
    }
    
    ~ScopedDebugMarker() 
    {
        mCommandBuffer->EndDebugGroup();
    }
    
private:
    CommandBufferPtr mCommandBuffer = nullptr;
};

NAMESPACE_RENDERCORE_END

#define DEBUGMAKRER_VAR_LINE(commandBuffer, name, color, line) _scopedDebugMarker_##line(commandBuffer, name, color)
#define EventVar(commandBuffer, name, color, n) DEBUGMAKRER_VAR_LINE(commandBuffer, name, color, n)
#define SCOPED_DEBUGMARKER_EVENT(commandBuffer, name, color) \
    RenderCore::ScopedDebugMarker EventVar(commandBuffer, name, color, __LINE__)

#endif /* GNX_ENGINE_COMMAND_BUFFER_INCLUDE_GPPP_H */
