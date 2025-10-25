//
//  GLShaderFunction.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#include "GLShaderFunction.h"
#include "GLShaderProgram.h"
#include "GLRenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

GLShaderFunction::GLShaderFunction(std::shared_ptr<const GLRenderDevice> renderDevicePtr)
{
    mRenderDevicePtr = renderDevicePtr;
    if (mRenderDevicePtr->getGLDeviceExtension()->isSupportedSeparateShader())
    {
        //mIsSeprateShader = true;
    }
}

GLShaderFunction::~GLShaderFunction()
{
    release();
}

ShaderFunctionPtr GLShaderFunction::initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage)
{
    if (nullptr == pShaderSource)
    {
        return nullptr;
    }
    
    if (mIsSeprateShader)
    {
        // Compile and link the separate vertex shader program, then read its uniform variable locations
        mShaderId = glCreateShaderProgramvEXT(::rendercore::getShaderStage(shaderStage), 1, &pShaderSource);
        
        if (!GetProgramLinkStatus(mShaderId))
        {
            glDeleteProgram(mShaderId);
            mShaderId = 0;
            return nullptr;
        }
    }
    
    else
    {
        mShaderSource = pShaderSource;
    }
    
    m_ShaderStage = shaderStage;
    
    return shared_from_this();
}

ShaderStage GLShaderFunction::getShaderStage() const
{
    return m_ShaderStage;
}

NAMESPACE_RENDERCORE_END
