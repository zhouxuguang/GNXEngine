//
//  GLGarbgeFactory.h
//  RenderEngine
//
//  Created by Zhou,Xuguang on 2019/4/30.
//  Copyright © 2019年 Zhou,Xuguang. All rights reserved.
//

#ifndef RENDERENGINE_GLGARBAGE_FACTORY_CDJNJDVJ_INCLUDE_H
#define RENDERENGINE_GLGARBAGE_FACTORY_CDJNJDVJ_INCLUDE_H

#include "GLRenderDefine.h"
#include <vector>
#include <mutex>

NAMESPACE_RENDERCORE_BEGIN

class GLGarbgeFactory
{
public:
    GLGarbgeFactory();
    
    ~GLGarbgeFactory();
    
    void postTexture(GLuint textureID);
    
    void postBuffer(GLuint bufferID);
    
    void postSampler(GLuint samplerID);
    
    void postShader(GLuint programID);
    
    void gc();
    
    void clear();
    
private:
    std::vector<GLuint> m_vecTextureIDs;
    std::vector<GLuint> m_vecBufferIDs;
    std::vector<GLuint> m_vecSamplerIDs;
    std::vector<GLuint> m_vecProgramIDs;
    
    std::mutex m_ResourceLock;
};

typedef std::weak_ptr<GLGarbgeFactory> GLGarbgeFactoryWeakPtr;
typedef std::shared_ptr<GLGarbgeFactory> GLGarbgeFactoryPtr;

NAMESPACE_RENDERCORE_END

#endif /* RENDERENGINE_GLGARBAGE_FACTORY_CDJNJDVJ_INCLUDE_H */
