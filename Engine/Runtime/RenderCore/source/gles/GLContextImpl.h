//
//  GLContextImpl.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/2.
//

#ifndef GNX_ENGINE_GL_CONTEXT_IMPL_INCLUDE_H
#define GNX_ENGINE_GL_CONTEXT_IMPL_INCLUDE_H

#include "GLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class GLContextImpl
{
public:
    explicit GLContextImpl(void *viewHandle){}
    
    virtual ~GLContextImpl() {}
    
    virtual void setFramebuffer() = 0;
    
    virtual bool presentFramebuffer() = 0;
    
private:
    
};

typedef std::shared_ptr<GLContextImpl> GLContextImplPtr;

GLContextImplPtr createEAGLContext(void *viewHandle);

GLContextImplPtr createEGLContext(void *viewHandle);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_CONTEXT_IMPL_INCLUDE_H */
