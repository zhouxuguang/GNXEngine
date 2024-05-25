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

NAMESPACE_RENDERCORE_BEGIN

// 计算缓冲区
class ComputeBuffer
{
public:
    ComputeBuffer(){}
    
    virtual ~ComputeBuffer(){}
    
    /**
     获取Buffer的长度
     @return buffer长度,单位btye
     */
    virtual uint32_t getBufferLength() const {return 0;}
    
    /**
     映射buffer数据
     @return buffer数据起始地址
     */
    virtual void* mapBufferData() const { return nullptr; }
    
    /**
     解除buffer数据
     */
    virtual void unmapBufferData(void* bufferData) const {}
    
    virtual bool isValid() const { return false; }
};

typedef std::shared_ptr<ComputeBuffer> ComputeBufferPtr;

NAMESPACE_RENDERCORE_END


#endif /* ComputeBuffer_h */
