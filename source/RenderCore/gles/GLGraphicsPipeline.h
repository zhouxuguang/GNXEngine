//
//  GLGraphicsPipeline.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#ifndef GNX_ENGINE_GL_GRAPHICS_PIPELINE_INCLUDE_HPP_FGDJK
#define GNX_ENGINE_GL_GRAPHICS_PIPELINE_INCLUDE_HPP_FGDJK

#include "GraphicsPipeline.h"
#include "GLRenderDefine.h"
#include "ShaderFunction.h"
#include "GLGPUProgram.h"

NAMESPACE_RENDERCORE_BEGIN

struct GLColorAttachmentDescriptor
{
    struct ColorMask
    {
        GLboolean red = GL_TRUE;
        GLboolean green = GL_TRUE;
        GLboolean blue = GL_TRUE;
        GLboolean aplha = GL_TRUE;
    };
    
    GLboolean blendingEnabled = false;
    GLuint sourceRGBBlendFactor = GL_ONE;
    GLuint destinationRGBBlendFactor = GL_ZERO;
    GLuint rgbBlendOperation = GL_FUNC_ADD;
    GLuint sourceAlphaBlendFactor = GL_ONE;
    GLuint destinationAplhaBlendFactor = GL_ZERO;
    GLuint aplhaBlendOperation = GL_FUNC_ADD;
    ColorMask colorMask;
};

struct GLStencilDescriptor
{
    GLuint stencilCompareFunction = GL_ALWAYS;
    GLuint stencilFailureOperation;
    GLuint depthFailureOperation;
    GLuint depthStencilPassOperation;
    uint32_t readMask;
    uint32_t writeMask;
    bool stencilEnable;
};

struct GLDepthDescriptor
{
    GLuint depthCompareFunction = GL_ALWAYS;
    GLboolean depthWriteEnabled = GL_FALSE;
};

class GLGraphicsPipeline : public GraphicsPipeline
{
public:
    GLGraphicsPipeline(const GraphicsPipelineDescriptor& des);
    
    ~GLGraphicsPipeline();
    
    virtual void attachVertexShader(ShaderFunctionPtr shaderFunction);
    
    virtual void attachFragmentShader(ShaderFunctionPtr shaderFunction);
    
    /**
     Call Opengl function to apply pipline state,
     NOTE: Must called in render thread!
     */
    void apply();
    
    void enableAttribute(int index);
    
    //opengl管线解绑前做的操作，主要是关闭顶点属性数组
    void unBind();
    
    void applyDepthStencil();
    
    void setReferenceValue(GLuint value);
    
    bool getGLVertextAttribDescriptor(int index, GLVertexDescriptor& des) const;
    
    uint32_t getUBOSize(uint32_t &bindingPoint) const;
    
private:
    void transToGLColorAttachment(const ColorAttachmentDescriptor& des);
    
    void transToGLDescriptor(const DepthStencilDescriptor &des);
    
    void transToGLVertexAttribDescriptor(const VertexDescriptor& des);
    
    void getGLVertexFormat(VertexFormat format, GLuint& count, GLenum& glFormat);
    
private:
    GLColorAttachmentDescriptor m_glPiplineColorAttachmentDescriptor;
    GLDepthDescriptor m_glDepthDecriptor;
    GLStencilDescriptor m_glStencilDescriptor;
    std::vector<GLVertexDescriptor> m_glVertexAttribDescrtiptor;
    GLGPUProgramUniquePtr m_glProgram = nullptr;
};

typedef std::shared_ptr<GLGraphicsPipeline> GLGraphicsPipelinePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_GRAPHICS_PIPELINE_INCLUDE_HPP_FGDJK */
