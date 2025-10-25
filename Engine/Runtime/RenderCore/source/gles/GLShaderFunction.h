//
//  GLShaderFunction.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#ifndef GNXENGINE_SHADER_FUNCTION_INCLUDE
#define GNXENGINE_SHADER_FUNCTION_INCLUDE

#include "ShaderFunction.h"
#include "GLRenderDefine.h"
#include "GLRenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

//class GLRenderDevice;

inline GLenum getShaderStage(ShaderStage shaderStage)
{
    GLenum stage = 0;
    switch (shaderStage)
    {
        case ShaderStage_Vertex:
            stage = GL_VERTEX_SHADER;
            break;
            
        case ShaderStage_Fragment:
            stage = GL_FRAGMENT_SHADER;
            break;
            
        default:
            break;
    }
    return stage;
}

class GLShaderFunction : public ShaderFunction, public std::enable_shared_from_this<GLShaderFunction>
{
public:
    GLShaderFunction(std::shared_ptr<const GLRenderDevice> renderDevicePtr);
    
    ~GLShaderFunction();
    
    virtual ShaderFunctionPtr initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage getShaderStage() const;
    
    GLuint getShaderID() const
    {
        return mShaderId;
    }
    
    std::string getShaderSource() const
    {
        return mShaderSource;
    }
    
    void release()
    {
        glDeleteShader(mShaderId);
        mShaderId = 0;
    }
    
private:    
    GLuint mShaderId = 0;      //seprate shader id
    ShaderStage m_ShaderStage;
    std::string mShaderSource;
    bool mIsSeprateShader = false;
    std::shared_ptr<const GLRenderDevice> mRenderDevicePtr = nullptr;
};

typedef std::shared_ptr<GLShaderFunction> GLShaderFunctionPtr;

NAMESPACE_RENDERCORE_END


#endif /* GNXENGINE_SHADER_FUNCTION_INCLUDE */
