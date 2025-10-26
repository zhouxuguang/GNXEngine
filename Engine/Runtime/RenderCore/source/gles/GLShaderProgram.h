//
//  GLShaderProgram.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/7.
//

#ifndef GNX_ENGINE_GLSHADER_PROGRAM_INCLUDE_H
#define GNX_ENGINE_GLSHADER_PROGRAM_INCLUDE_H

#include "GLRenderDefine.h"
#include <unordered_map>
#include <vector>
#include <string>

NAMESPACE_RENDERCORE_BEGIN

//seprate shader object

class GLShaderProgram
{
public:
    GLShaderProgram();
    
    GLShaderProgram(GLenum type, const char* pShaderSource);
    
    ~GLShaderProgram();
    
    GLuint GetProgramID() const;
    
private:
    GLuint mProgram = 0;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLSHADER_PROGRAM_INCLUDE_H */
