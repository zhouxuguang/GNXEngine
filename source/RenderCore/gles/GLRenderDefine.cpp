//
//  gl_RenderDefine.cpp
//
//
//  Created by zhouxuguang on 2019/7/31.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#include "GLRenderDefine.h"
#include "gl3stub.h"
#include <iostream>

NAMESPACE_RENDERCORE_BEGIN

#ifdef _OPENGL_CHECK_ERROR_

void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::string errStr = gl_error_string(error);
        std::cout << "OpenGL Error: " << errStr << " " << function << " " << file <<" " << line << std::endl;
        return false;
    }
    return true;
}

#endif


struct GL_Version
{
    int major;
    int minor;
};

static void getCurrentContextVersion(GL_Version& version)
{
    int major = 0;
    int minor = 0;
    const char* pcVer = (const char*)glGetString(GL_VERSION);
    
    if (!pcVer)
    {
        return;
    }
    
    sscanf(pcVer, "OpenGL ES %u.%u", &major, &minor);
    
    version.major = major;
    version.minor = minor;
}

bool OpenGLESContext::s_bIsSupportGLES30(false);
std::once_flag OpenGLESContext::s_gles30OnceFlag;
int OpenGLESContext::s_nMajor(0);
int OpenGLESContext::s_nMinor(0);

bool OpenGLESContext::isSupportGLES30()
{
    std::call_once(s_gles30OnceFlag, []
                                     {
                                        s_bIsSupportGLES30 = gl3stubInit();
                                     });
    
    return (s_nMajor >= 3) && s_bIsSupportGLES30;
}

void OpenGLESContext::initCurrentContext()
{
    GL_Version version;
    getCurrentContextVersion(version);
    s_nMajor = version.major;
    s_nMinor = version.minor;
}

int OpenGLESContext::getVersionNumber()
{
    return s_nMajor * 10 + s_nMinor;
}

bool GetProgramLinkStatus(GLuint program)
{
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        //ALOGE("CompileComputeShaderString Could not link program");
        GLint infoLogLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen)
        {
            GLchar* infoLog = (GLchar*)malloc(infoLogLen);
            if (infoLog)
            {
                glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
                //ALOGE("CompileComputeShaderString Could not link program:\n%s\n", infoLog);
                free(infoLog);
            }
        }
        
        return false;
    }
    
    return true;
}

bool CompileShader(GLuint *shader, GLenum type, const GLchar* source)
{
    if (nullptr == shader)
    {
        return false;
    }
    
    GLint status = 0;
    if (!source)
    {
        return false;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    
    if (!status)
    {
        GLsizei length = 0;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0)
        {
            GLchar* src = (GLchar *)malloc(sizeof(GLchar) * length);
            glGetShaderInfoLog(*shader, length, nullptr, src);
            free(src);
        }
        else
        {
        }
        
        return false;
    }
    
    return true;
}

NAMESPACE_RENDERCORE_END


