//
//  GLRenderDefine.h
//
//
//  Created by zhouxuguang on 2019/7/18.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#ifndef GNX_ENGINE_RENDER_DEFINE_INCLUDE_H
#define GNX_ENGINE_RENDER_DEFINE_INCLUDE_H

#include "RenderDefine.h"
#include "BaseLib/DebugBreaker.h"

#if defined( __ANDROID__ )
#include <GLES2/gl2.h>
//#define GL_GLEXT_PROTOTYPES
//#include <GLES2/gl2ext.h>
#elif defined(__APPLE__)
#define GLES_SILENCE_DEPRECATION
#import <OpenGLES/ES2/gl.h>
//#import <OpenGLES/ES2/glext.h>
#endif

#include "glesw.h"
#include "gl3stub.h"

#include <string>
#include <mutex>

NAMESPACE_RENDERCORE_BEGIN


inline char const* gl_error_string(GLenum const err)
{
    switch (err)
    {
        // opengl 2 errors (8)
        case GL_NO_ERROR:
            return "GL_NO_ERROR";

        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";

        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";

        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";

        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";

            // opengl 3 errors (1)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";

        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";

        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";

        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";

        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";

            // gles 2, 3 and gl 4 error are handled by the switch above
        default:
            //assert(!"unknown error");
            return nullptr;
    }
}

#define _OPENGL_CHECK_ERROR_

#ifdef _OPENGL_CHECK_ERROR_
    //intrinsic to a compiler
    #define ASSERT(x) if(!(x)) baselib::DebugBreak();
    #define GLCALL(x) GLClearError();\
        x;\
        ASSERT(GLLogCall(#x, __FILE__, __LINE__))

    void GLClearError();
    bool GLLogCall(const char* function, const char* file, int line);
#else
    #define ASSERT(x) ((void)0)
    #define GLCALL(x) x;
#endif

class OpenGLESContext
{
private:
    static bool s_bIsSupportGLES30;
    static std::once_flag s_gles30OnceFlag;
    static int s_nMajor;
    static int s_nMinor;
public:
    static bool isSupportGLES30();
    
    static void initCurrentContext();
    
    static int getVersionNumber();
};

bool GetProgramLinkStatus(GLuint program);

bool CompileShader(GLuint *shader, GLenum type, const GLchar* source);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_DEFINE_INCLUDE_H */
