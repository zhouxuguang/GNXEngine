//
//  FrameBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_FRAME_BUFFER_INCLUDE
#define GNX_ENGINE_FRAME_BUFFER_INCLUDE

#include "Texture2D.h"

NAMESPACE_RENDERCORE_BEGIN

class FrameBuffer
{
public:
    FrameBuffer();
    
    FrameBuffer(uint32_t width, uint32_t height);
    
    virtual ~FrameBuffer();
    
    virtual uint32_t GetWidth() const = 0;
    
    virtual uint32_t GetHeight() const = 0;
};

typedef std::shared_ptr<FrameBuffer> FrameBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_FRAME_BUFFER_INCLUDE */
