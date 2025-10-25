//
//  GLGPUProgram.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/4.
//

#ifndef RENDERCORE_GL_PROGRAM_INCLUDE_EFSKEJFGJ
#define RENDERCORE_GL_PROGRAM_INCLUDE_EFSKEJFGJ


#include "GLRenderDefine.h"
#include "GLESProgramUtil.h"
#include <set>

NAMESPACE_RENDERCORE_BEGIN

class GLShaderFunction;
typedef std::shared_ptr<GLShaderFunction> GLShaderFunctionPtr;

class GLRenderDevice;

class GLGPUProgram
{
public:
    GLGPUProgram(std::shared_ptr<GLRenderDevice> renderDevicePtr);
    
    ~GLGPUProgram();
    
    void attachShader(GLShaderFunctionPtr shaderFunction);
    
    bool hasLinked() const
    {
        return m_bLinked;
    }
    
    void link();
    
    void apply();
    
    void enableAttribute(int index);
    
    void unBind();
    
    GLint getUBOSize(GLuint &bindingPoint) const;
    
private:
    GLuint m_ProgramID = 0;
    std::set<int> mAttriIndexs;
    bool m_bLinked = false;
    std::vector<GLVertexDescriptor> m_vecAttrDesc;
    std::vector<GLUBODescriptor> m_vecUBODesc;
    
    std::vector<GLShaderFunctionPtr> m_vecShaderFunctions;
    
    std::shared_ptr<GLRenderDevice> mRenderDevicePtr = nullptr;
    bool mIsSeprateShader = false;
};

typedef std::unique_ptr<GLGPUProgram> GLGPUProgramUniquePtr;
typedef std::shared_ptr<GLGPUProgram> GLGPUProgramPtr;

NAMESPACE_RENDERCORE_END

#endif /* RENDERCORE_GL_PROGRAM_INCLUDE_EFSKEJFGJ */
