//
//  ComputeBuffer.h
//  GNXENGINE
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_COMPUTE_BUFFER_DGDFGFKSG
#define GNX_ENGINE_COMPUTE_BUFFER_DGDFGFKSG

#include "RenderDefine.h"
#include "GraphicsPipeline.h"
#include "VertexBuffer.h"
#include "RCTexture.h"

NAMESPACE_RENDERCORE_BEGIN

// 计算缓冲区
class ComputeBuffer : public VertexBuffer
{
public:
    ComputeBuffer(){}
    
    virtual ~ComputeBuffer(){}
    
    /**
     获取Buffer的长度
     @return buffer长度,单位btye
     */
    virtual uint32_t GetBufferLength() const {return 0;}
    
    /**
     映射buffer数据
     @return buffer数据起始地址
     */
    virtual void* MapBufferData() const { return nullptr; }
    
    /**
     解除buffer数据
     */
    virtual void UnmapBufferData(void* bufferData) const {}
    
    virtual bool IsValid() const { return false; }
    
    /**
      设置名字
     */
    virtual void SetName(const char* name) {};
    
    /**
     * @brief 获取缓冲区的当前状态
     */
    virtual ResourceState GetState() const = 0;
    
    /**
     * @brief 设置缓冲区的当前状态
     */
    virtual void SetState(const ResourceState& state) = 0;
    
    /**
     * @brief 在读取前插入资源屏障
     * @param commandBuffer 命令缓冲区（平台特定的句柄）
     * @param access 访问类型
     * @param stage 管线阶段
     */
    virtual void PreReadBarrier(void* commandBuffer, ResourceAccess access,
                             ResourcePipelineStage stage) = 0;
    
    /**
     * @brief 在写入前插入资源屏障
     * @param commandBuffer 命令缓冲区（平台特定的句柄）
     * @param access 访问类型
     * @param stage 管线阶段
     */
    virtual void PreWriteBarrier(void* commandBuffer, ResourceAccess access,
                             ResourcePipelineStage stage) = 0;
};

typedef std::shared_ptr<ComputeBuffer> ComputeBufferPtr;

NAMESPACE_RENDERCORE_END


#endif /* ComputeBuffer_h */
