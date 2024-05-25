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
    virtual RenderEncoderPtr createDefaultRenderEncoder() const = 0;
    
    virtual RenderEncoderPtr createRenderEncoder(const RenderPass& renderPass) const = 0;
    
    virtual ComputeEncoderPtr createComputeEncoder() const = 0;
    
    //呈现到屏幕上，上屏
    virtual void presentFrameBuffer() = 0;
    
    //等待命令缓冲区执行完成
    virtual void waitUntilCompleted() = 0;
};

typedef std::shared_ptr<CommandBuffer> CommandBufferPtr;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_COMMAND_BUFFER_INCLUDE_GPPP_H */
