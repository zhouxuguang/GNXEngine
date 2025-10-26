//
//  GLShaderProgram.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/7.
//

#include "GLShaderProgram.h"

NAMESPACE_RENDERCORE_BEGIN

GLShaderProgram::GLShaderProgram()
{
    //
}

GLShaderProgram::GLShaderProgram(GLenum type, const char* pShaderSource)
{
    // Compile and link the separate vertex shader program, then read its uniform variable locations
    mProgram = glCreateShaderProgramvEXT(type, 1, &pShaderSource);
    
    if (!GetProgramLinkStatus(mProgram))
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
}

GLShaderProgram::~GLShaderProgram()
{
    if (mProgram)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
}

GLuint GLShaderProgram::GetProgramID() const
{
    return mProgram;
}

NAMESPACE_RENDERCORE_END
