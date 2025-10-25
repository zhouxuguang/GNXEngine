//
//  GLGPUProgram.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/4.
//

#include "GLGPUProgram.h"
#include "GLShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

GLGPUProgram::GLGPUProgram(std::shared_ptr<GLRenderDevice> renderDevicePtr)
{
    mRenderDevicePtr = renderDevicePtr;
    if (mRenderDevicePtr->getGLDeviceExtension()->isSupportedSeparateShader())
    {
        mIsSeprateShader = true;
    }
    m_ProgramID = glCreateProgram();
}

GLGPUProgram::~GLGPUProgram()
{
    if (m_ProgramID)
    {
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    }
}

void GLGPUProgram::attachShader(GLShaderFunctionPtr shaderFunction)
{
    m_vecShaderFunctions.push_back(shaderFunction);
    //glAttachShader(m_ProgramID, shaderFunction->getShaderID());
    //shaderFunction->release();
}

void GLGPUProgram::link()
{
    if (m_bLinked)
    {
        return;
    }
    
    std::vector<GLuint> shaders;
    for (auto &iter : m_vecShaderFunctions)
    {
        GLuint shader = 0;
        if (!CompileShader(&shader, getShaderStage(iter->getShaderStage()), iter->getShaderSource().c_str()))
        {
            glDeleteShader(shader);
            return;
        }
        glAttachShader(m_ProgramID, shader);
        shaders.push_back(shader);
    }
    
    glLinkProgram(m_ProgramID);
    if (!GetProgramLinkStatus(m_ProgramID))
    {
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
        return;
    }
    m_bLinked = true;
    m_vecShaderFunctions.clear();
    
    for (size_t i = 0; i < shaders.size(); i ++)
    {
        glDeleteShader(shaders[i]);
    }
    
    glUseProgram(m_ProgramID);
    
    GetProgramAttributeInfo(m_ProgramID, m_vecAttrDesc);
    GetProgramUniformBlockInfo(m_ProgramID, m_vecUBODesc);
    GetProgramUniformInfo(m_ProgramID);
}

void GLGPUProgram::apply()
{
    glUseProgram(m_ProgramID);
    
//    for (int i = 0; i < m_vecAttrDesc.size(); i ++)
//    {
//        glEnableVertexAttribArray(i);
//    }
}

void GLGPUProgram::enableAttribute(int index)
{
    glEnableVertexAttribArray(index);
    mAttriIndexs.insert(index);
}

void GLGPUProgram::unBind()
{
    for (const auto& iter : mAttriIndexs)
    {
        glDisableVertexAttribArray(iter);
    }
    mAttriIndexs.clear();
    glUseProgram(0);
}

GLint GLGPUProgram::getUBOSize(GLuint &bindingPoint) const
{
    //GLint index = (GLint)(m_vecUBODesc.size() - bindingPoint);
    bindingPoint = bindingPoint - m_vecAttrDesc.size();
    if (bindingPoint < 0 || bindingPoint >= m_vecUBODesc.size())
    {
        return -1;
    }
    
    return m_vecUBODesc[bindingPoint].dataSize;
}

NAMESPACE_RENDERCORE_END
