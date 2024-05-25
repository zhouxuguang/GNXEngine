//
//  GLCommandBuffer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#include "GLCommandBuffer.h"
#include "GLRenderEncoder.h"
#include "GLRenderTexture.h"
#include "GLFrameBuffer.h"
#include "GLFrameBufferCache.h"

NAMESPACE_RENDERCORE_BEGIN

GLCommandBuffer::GLCommandBuffer(GLDrawStatePtr drawState, GLContextImplPtr glContext, uint32_t width, uint32_t height)
{
    m_drawState = drawState;
    m_glContext = glContext;
    
    m_width = width;
    m_height = height;
}

GLCommandBuffer::~GLCommandBuffer()
{
}

RenderEncoderPtr GLCommandBuffer::createDefaultRenderEncoder() const
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_glContext->setFramebuffer();
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glClearDepthf(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    
    glViewport(0, 0, m_width, m_height);
    
    GLRenderEncoderPtr renderEncoder = std::make_shared<GLRenderEncoder>(m_drawState);
    
    return renderEncoder;
}

RenderEncoderPtr GLCommandBuffer::createRenderEncoder(const RenderPass& renderPass) const
{
    GLFrameBufferCache* pCacheInstance = GLFrameBufferCache::getInstance();
    FboCacheItem cacheItem;
    cacheItem.width = m_width;
    cacheItem.height = m_height;
    
    GLFrameBufferPtr frameBufferPtr = pCacheInstance->GetCacheFBO(cacheItem);
    if (nullptr == frameBufferPtr)
    {
        frameBufferPtr = std::make_shared<GLFrameBuffer>(m_width, m_height);
        pCacheInstance->AddFBO(cacheItem, frameBufferPtr);
    }
    
    frameBufferPtr->apply();
    
    //先解绑所有的渲染缓冲区
    for (size_t i = 0; i < 16; i ++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, 0, 0);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    
    //进行绑定
    std::vector<GLenum> attachments;
    for (size_t i = 0; i < renderPass.colorAttachments.size(); i ++)
    {
        RenderPassColorAttachmentPtr iter = renderPass.colorAttachments[i];
        if (!iter)
        {
            continue;
        }
        
        GLRenderTexturePtr glRenderTexture = std::dynamic_pointer_cast<GLRenderTexture>(iter->texture);
        if (glRenderTexture == nullptr)
        {
            continue;
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, glRenderTexture->getTextureID(), 0);
        attachments.push_back((GLenum)(GL_COLOR_ATTACHMENT0 + i));
        glClearColor(iter->clearColor.red, iter->clearColor.green, iter->clearColor.blue, iter->clearColor.alpha);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    if (renderPass.depthAttachment)
    {
        GLRenderTexturePtr glRenderTexture = std::dynamic_pointer_cast<GLRenderTexture>(renderPass.depthAttachment->texture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, glRenderTexture->getTextureID(), 0);
        
        glClearDepthf(renderPass.depthAttachment->clearDepth);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    
    if (renderPass.stencilAttachment)
    {
        GLRenderTexturePtr glRenderTexture = std::dynamic_pointer_cast<GLRenderTexture>(renderPass.stencilAttachment->texture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, glRenderTexture->getTextureID(), 0);
        glClearStencil(renderPass.stencilAttachment->clearStencil);
        glClear(GL_STENCIL_BUFFER_BIT);
    }
    
    //设置写入的buffer
    if (attachments.empty())
    {
        const GLenum attachments[1] = { GL_NONE};
        glDrawBuffers(1, attachments);   //glDrawBuffer  3.0没有，需要用glDrawBuffers
        glReadBuffer(GL_NONE);
    }

    else
    {
        glDrawBuffers((GLsizei)attachments.size(), attachments.data());
    }
    
    //设置渲染区域
    glViewport(renderPass.renderRegion.offsetX, renderPass.renderRegion.offsetY, renderPass.renderRegion.width, renderPass.renderRegion.height);
    
    GLRenderEncoderPtr renderEncoder = std::make_shared<GLRenderEncoder>(m_drawState);
    
    return renderEncoder;
}

void GLCommandBuffer::presentFrameBuffer()
{
    assert(m_glContext);
    
    if (m_glContext)
    {
        m_glContext->presentFramebuffer();
    }
}

NAMESPACE_RENDERCORE_END
