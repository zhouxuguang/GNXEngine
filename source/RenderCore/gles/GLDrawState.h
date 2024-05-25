//
//  GLDrawState.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/4.
//

#ifndef GNX_ENGINE_GL_DRAWSTATE_INCLUDE_HH
#define GNX_ENGINE_GL_DRAWSTATE_INCLUDE_HH

#include "GLGraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

class GLDrawState
{
public:
    GLDrawState(){}
    
    ~GLDrawState(){}
    
public:
    GLGraphicsPipelinePtr pipeline = nullptr;
};

typedef std::shared_ptr<GLDrawState> GLDrawStatePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_DRAWSTATE_INCLUDE_HH */
