//
//  GLFrameBuffer.hpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_GLES_FRAME_BUFFER_INCLUDE
#define GNX_ENGINE_GLES_FRAME_BUFFER_INCLUDE

#include "GLRenderDefine.h"
#include "FrameBuffer.h"
#include "gl3stub.h"
#include "GLTexture2D.h"

NAMESPACE_RENDERCORE_BEGIN

class GLFrameBuffer : public FrameBuffer
{
public:
    GLFrameBuffer(uint32_t width, uint32_t height);
    
    ~GLFrameBuffer();
    
    virtual uint32_t GetWidth() const;
    
    virtual uint32_t GetHeight() const;
    
//    virtual void attachColorBuffer(Texture2dPtr colorBuffer);
//    
//    virtual void attachDepthBuffer(Texture2dPtr depthBuffer);
//    
//    virtual void attachDepthStencilBuffer(Texture2dPtr depthStencilBuffer);
//    
    void apply();
    
private:
    GLuint m_bufferId = 0;
    std::vector<GLTexture2DPtr> m_colorBuffers;
    GLTexture2DPtr m_depthBuffer = nullptr;
    GLTexture2DPtr m_depthStencilBuffer = nullptr;
    
    uint32_t mWidth;
    uint32_t mHeight;
};

typedef std::shared_ptr<GLFrameBuffer> GLFrameBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLES_FRAME_BUFFER_INCLUDE */
