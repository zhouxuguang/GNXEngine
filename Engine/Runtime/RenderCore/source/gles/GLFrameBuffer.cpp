//
//  GLFrameBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#include "GLFrameBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

GLFrameBuffer::GLFrameBuffer(uint32_t width, uint32_t height) : mWidth(width), mHeight(height)
{
    glGenFramebuffers(1, &m_bufferId);
    //glBindFramebuffer(GL_FRAMEBUFFER, m_bufferId);
}

GLFrameBuffer::~GLFrameBuffer()
{
    if (m_bufferId != 0)
    {
        glDeleteFramebuffers(1, &m_bufferId);
    }
}

//void GLFrameBuffer::attachColorBuffer(Texture2dPtr colorBuffer)
//{
//    GLTexture2dPtr colorPtr = std::dynamic_pointer_cast<GLTexture2d>(colorBuffer);
//    if (!colorPtr || !colorPtr->isValid())
//    {
//        return;
//    }
//    m_colorBuffers.push_back(colorPtr);
//}
//
//void GLFrameBuffer::attachDepthBuffer(Texture2dPtr depthBuffer)
//{
//    GLTexture2dPtr depthPtr = std::dynamic_pointer_cast<GLTexture2d>(depthBuffer);
//    if (!depthPtr || !depthPtr->isValid())
//    {
//        return;
//    }
//
//    m_depthBuffer = depthPtr;
//}
//
//void GLFrameBuffer::attachDepthStencilBuffer(Texture2dPtr depthStencilBuffer)
//{
//    GLTexture2dPtr depthStencilPtr = std::dynamic_pointer_cast<GLTexture2d>(depthStencilBuffer);
//    if (!depthStencilPtr || !depthStencilPtr->isValid())
//    {
//        return;
//    }
//    m_depthStencilBuffer = depthStencilPtr;
//}
//
void GLFrameBuffer::apply()
{
    //glIsFramebuffer的问题，就是没有绑定之前，都是返回false
    //If framebuffer is a name returned by glGenFramebuffers, by that has not yet been bound through a call
    //to glBindFramebuffer, then the name is not a framebuffer object and glIsFramebuffer returns GL_FALSE.
    if (m_bufferId == 0 /*|| !glIsFramebuffer(m_bufferId)*/)
    {
        return;
    }

    // bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_bufferId);
}

uint32_t GLFrameBuffer::GetWidth() const
{
    return mWidth;
}

uint32_t GLFrameBuffer::GetHeight() const
{
    return mHeight;
}

NAMESPACE_RENDERCORE_END
