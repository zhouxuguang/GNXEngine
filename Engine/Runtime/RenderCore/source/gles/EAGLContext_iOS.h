//
//  EAGLContext_iOS.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/25.
//

#ifndef GLES_RENDERCORE_RAGLCONTEXT_INCLUDE_H
#define GLES_RENDERCORE_RAGLCONTEXT_INCLUDE_H

#include "GLRenderDefine.h"
#import <Foundation/Foundation.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/QuartzCore.h>
#include "GLContextImpl.h"

NAMESPACE_RENDERCORE_BEGIN

class EAGLContext_iOS : public GLContextImpl
{
public:
    explicit EAGLContext_iOS(void *viewHandle);
    
private:
    void createFramebuffer();
    
    void deleteFramebuffer();
    
    void setFramebuffer();
    
    bool presentFramebuffer();
    
    void setupDisplayLink();
    
private:
    // The pixel dimensions of the CAEAGLLayer.
    GLint m_framebufferWidth = 0;
    GLint m_framebufferHeight = 0;
    EAGLContext *m_GLContext = nil;
    CAEAGLLayer *m_EAGLLayer = nil;
    bool m_isGLES3 = true;
    
    // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view.
    GLuint m_defaultFramebuffer = 0;
    GLuint m_colorRenderbuffer = 0;
    GLuint m_depthRenderbuffer = 0;
    GLuint m_msaaFramebuffer = 0;
    GLuint m_msaaRenderbuffer = 0;
    GLuint m_msaaDepthbuffer = 0;
    
    CADisplayLink* m_displayLink = nil;
};

NAMESPACE_RENDERCORE_END

#endif
