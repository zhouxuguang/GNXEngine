//
//  GLCommandBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#ifndef GNX_GL_RENDERCOMMAND_INCLUDE_H
#define GNX_GL_RENDERCOMMAND_INCLUDE_H

#include "CommandBuffer.h"
#include "GLDrawState.h"
#include "GLContextImpl.h"

NAMESPACE_RENDERCORE_BEGIN

class GLCommandBuffer : public CommandBuffer
{
public:
    GLCommandBuffer(GLDrawStatePtr drawState, GLContextImplPtr glContext, uint32_t width, uint32_t height);
    
    ~GLCommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    virtual RenderEncoderPtr createDefaultRenderEncoder() const;
    
    virtual RenderEncoderPtr createRenderEncoder(const RenderPass& renderPass) const;
    
    virtual void presentFrameBuffer();
    
private:
    //GLRenderContextPtr m_renderContext = nullptr;
    GLDrawStatePtr m_drawState = nullptr;
    GLContextImplPtr m_glContext = nullptr;
    
    //mutable GLuint m_frameBuffer = 0;
    
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

typedef std::shared_ptr<GLCommandBuffer> GLCommandBufferPtr;


NAMESPACE_RENDERCORE_END

#endif /* GNX_GL_RENDERCOMMAND_INCLUDE_H */
