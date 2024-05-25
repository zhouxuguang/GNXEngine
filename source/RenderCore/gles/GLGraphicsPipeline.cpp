//
//  GLGraphicsPipeline.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#include "GLGraphicsPipeline.h"
#include "GLShaderFunction.h"
#include "BaseLib/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

GLGraphicsPipeline::GLGraphicsPipeline(const GraphicsPipelineDescriptor& des) : GraphicsPipeline(des)
{
    m_glProgram = std::make_unique<GLGPUProgram>(std::dynamic_pointer_cast<GLRenderDevice>(getRenderDevice()));
    transToGLColorAttachment(des.colorAttachmentDescriptor);
    transToGLVertexAttribDescriptor(des.vertexDescriptor);
}

GLGraphicsPipeline::~GLGraphicsPipeline()
{
    m_glProgram.reset();
}

void GLGraphicsPipeline::attachVertexShader(ShaderFunctionPtr shaderFunction)
{
    GLShaderFunctionPtr glShaderPtr = std::dynamic_pointer_cast<GLShaderFunction>(shaderFunction);
    if (m_glProgram && glShaderPtr)
    {
        m_glProgram->attachShader(glShaderPtr);
    }
}

void GLGraphicsPipeline::attachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    GLShaderFunctionPtr glShaderPtr = std::dynamic_pointer_cast<GLShaderFunction>(shaderFunction);
    if (m_glProgram && glShaderPtr)
    {
        m_glProgram->attachShader(glShaderPtr);
    }
}

void GLGraphicsPipeline::apply()
{
    if (m_glProgram)
    {
        if (!m_glProgram->hasLinked())
        {
            m_glProgram->link();
        }
        m_glProgram->apply();
    }
    
    if (m_glPiplineColorAttachmentDescriptor.blendingEnabled)
    {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(m_glPiplineColorAttachmentDescriptor.rgbBlendOperation,
                                m_glPiplineColorAttachmentDescriptor.aplhaBlendOperation);
        glBlendFuncSeparate(m_glPiplineColorAttachmentDescriptor.sourceRGBBlendFactor,
                            m_glPiplineColorAttachmentDescriptor.destinationRGBBlendFactor,
                            m_glPiplineColorAttachmentDescriptor.sourceAlphaBlendFactor,
                            m_glPiplineColorAttachmentDescriptor.destinationAplhaBlendFactor);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    
    glColorMask(m_glPiplineColorAttachmentDescriptor.colorMask.red,
                m_glPiplineColorAttachmentDescriptor.colorMask.green,
                m_glPiplineColorAttachmentDescriptor.colorMask.blue,
                m_glPiplineColorAttachmentDescriptor.colorMask.aplha);
    
    applyDepthStencil();
}

void GLGraphicsPipeline::enableAttribute(int index)
{
    if (m_glProgram)
    {
        m_glProgram->enableAttribute(index);
    }
}

void GLGraphicsPipeline::unBind()
{
    if (m_glProgram)
    {
        m_glProgram->unBind();
    }
}

void GLGraphicsPipeline::transToGLDescriptor(const DepthStencilDescriptor &des)
{
    auto getGLCompareFunction = [](CompareFunction compare) -> GLuint {
        GLuint glCompareFunc = GL_ALWAYS;
        switch (compare) {
            case CompareFunctionNever:
                glCompareFunc = GL_NEVER;
                break;
             case CompareFunctionLess:
                glCompareFunc = GL_LESS;
                break;
            case CompareFunctionEqual:
                glCompareFunc = GL_EQUAL;
                break;
            case CompareFunctionLessThanOrEqual:
                glCompareFunc = GL_LEQUAL;
                break;
            case CompareFunctionGreater:
                glCompareFunc = GL_GREATER;
                break;
            case CompareFunctionNotEqual:
                glCompareFunc = GL_NOTEQUAL;
                break;
            case CompareFunctionGreaterThanOrEqual:
                glCompareFunc = GL_GEQUAL;
                break;
            case CompareFunctionAlways:
                glCompareFunc = GL_ALWAYS;
                break;
            default:
                break;
        }
        return glCompareFunc;
    };

    auto getGLOperation = [](StencilOperation operation) -> GLuint {
        GLuint glOperation;
        switch (operation) {
            case StencilOperationZero:
                glOperation = GL_ZERO;
                break;
            case StencilOperationInvert:
                glOperation = GL_INVERT;
                break;
            case StencilOperationKeep:
                glOperation = GL_KEEP;
                break;
            case StencilOperationReplace:
                glOperation = GL_REPLACE;
                break;
            case StencilOperationIncrementClamp:
                glOperation = GL_INCR;
                break;
            case StencilOperationDecrementClamp:
                glOperation = GL_DECR;
                break;
            case StencilOperationIncrementWrap:
                glOperation = GL_INCR_WRAP;
                break;
            case StencilOperationDecrementWrap:
                glOperation = GL_DECR_WRAP;
                break;
            default:
                break;
        }
        return glOperation;
    };
    m_glDepthDecriptor.depthCompareFunction = getGLCompareFunction(des.depthCompareFunction);
    m_glDepthDecriptor.depthWriteEnabled = des.depthWriteEnabled;
    m_glStencilDescriptor.stencilEnable = des.stencil.stencilEnable;
    m_glStencilDescriptor.stencilCompareFunction = getGLCompareFunction(des.stencil.stencilCompareFunction);
    m_glStencilDescriptor.depthStencilPassOperation = getGLOperation(des.stencil.depthStencilPassOperation);
    m_glStencilDescriptor.depthFailureOperation = getGLOperation(des.stencil.depthFailureOperation);
    m_glStencilDescriptor.stencilFailureOperation = getGLOperation(des.stencil.stencilFailureOperation);
    m_glStencilDescriptor.readMask = des.stencil.readMask;
    m_glStencilDescriptor.writeMask = des.stencil.writeMask;
}

void GLGraphicsPipeline::transToGLColorAttachment(const ColorAttachmentDescriptor &des)
{
    auto getGLBlendFactor = [](BlendFactor factor)->GLuint{
        GLuint glFactor = GL_ONE;
        switch (factor) {
            case BlendFactor::BlendFactorOne:
                glFactor = GL_ONE;
                break;
            case BlendFactor::BlendFactorZero:
                glFactor = GL_ZERO;
                break;
            case BlendFactor::BlendFactorSourceAlpha:
                glFactor = GL_SRC_ALPHA;
                break;
            case BlendFactor::BlendFactorOneMinusSourceAlpha:
                glFactor = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case BlendFactor::BlendFactorDestinationAlpha:
                glFactor = GL_DST_ALPHA;
                break;
            case BlendFactor::BlendFactorOneMinusDestinationAlpha:
                glFactor = GL_ONE_MINUS_DST_ALPHA;
                break;
            case BlendFactor::BlendFactorDestinationColor:
                glFactor = GL_DST_COLOR;
                break;
            case BlendFactor::BlendFactorOneMinusDestinationColor:
                glFactor = GL_ONE_MINUS_DST_COLOR;
                break;
            case BlendFactor::BlendFactorSourceAlphaSaturated:
                glFactor = GL_SRC_ALPHA_SATURATE;
                break;
            case BlendFactor::BlendFactorBlendColor:
                glFactor = GL_CONSTANT_COLOR;
                break;
            case BlendFactor::BlendFactorOneMinusBlendColor:
                glFactor = GL_ONE_MINUS_CONSTANT_COLOR;
                break;
            case BlendFactor::BlendFactorBlendAlpha:
                glFactor = GL_CONSTANT_ALPHA;
                break;
            case BlendFactor::BlendFactorOneMinusBlendAlpha:
                glFactor = GL_ONE_MINUS_CONSTANT_ALPHA;
                break;
            case BlendFactor::BlendFactorSourceColor:
                glFactor = GL_SRC_COLOR;
                break;
            default:
                break;
        }
        return glFactor;
    };
    
    auto getGLBlendOperator = [](BlendEquation equation) -> GLuint {
        GLuint glOperation = GL_FUNC_ADD;
        switch (equation) {
            case BlendEquation::BlendEquationAdd:
                glOperation = GL_FUNC_ADD;
                break;
            case BlendEquation::BlendEquationSubtract:
                glOperation = GL_FUNC_SUBTRACT;
                break;
            case BlendEquation::BlendEquationReverseSubtract:
                glOperation = GL_FUNC_REVERSE_SUBTRACT;
                break;
            default:
                break;
        }
        return glOperation;
    };
    
    m_glPiplineColorAttachmentDescriptor.blendingEnabled = des.blendingEnabled;
    
    if (des.blendingEnabled)
    {
        m_glPiplineColorAttachmentDescriptor.sourceRGBBlendFactor = getGLBlendFactor(des.sourceRGBBlendFactor);
        m_glPiplineColorAttachmentDescriptor.destinationRGBBlendFactor = getGLBlendFactor(des.destinationRGBBlendFactor);
        m_glPiplineColorAttachmentDescriptor.sourceAlphaBlendFactor = getGLBlendFactor(des.sourceAlphaBlendFactor);
        m_glPiplineColorAttachmentDescriptor.destinationAplhaBlendFactor = getGLBlendFactor(des.destinationAplhaBlendFactor);
        m_glPiplineColorAttachmentDescriptor.rgbBlendOperation = getGLBlendOperator(des.rgbBlendOperation);
        m_glPiplineColorAttachmentDescriptor.aplhaBlendOperation = getGLBlendOperator(des.aplhaBlendOperation);
    }
    
    if (des.writeMask != ColorWriteMask::ColorWriteMaskAll)
    {
        m_glPiplineColorAttachmentDescriptor.colorMask.red = des.writeMask & ColorWriteMask::ColorWriteMaskRed;
        m_glPiplineColorAttachmentDescriptor.colorMask.green = des.writeMask & ColorWriteMask::ColorWriteMaskGreen;
        m_glPiplineColorAttachmentDescriptor.colorMask.blue = des.writeMask & ColorWriteMask::ColorWriteMaskBlue;
        m_glPiplineColorAttachmentDescriptor.colorMask.aplha = des.writeMask &  ColorWriteMask::ColorWriteMaskAlpha;
    }
}

void GLGraphicsPipeline::transToGLVertexAttribDescriptor(const VertexDescriptor &des)
{
    if (des.attributes.size() != des.layouts.size())
    {
        baselib::LogService::InfoPrint("des.attributes and des.layouts must be same");
        assert(false);
        return;
    }
    
    for (size_t i = 0; i < des.attributes.size(); i ++)
    {
        const VertextAttributesDescritptor &attr = des.attributes[i];
        
        GLVertexDescriptor vertexAttrib;
        vertexAttrib.index = attr.index;
        GLuint size = 0;
        GLenum type = 0;
        getGLVertexFormat(attr.format, size, type);
        vertexAttrib.size = size;
        vertexAttrib.type = type;
        vertexAttrib.stride = des.layouts[i].stride;
        vertexAttrib.offset = attr.offset;
        vertexAttrib.normalized = GL_FALSE;
        m_glVertexAttribDescrtiptor.push_back(vertexAttrib);
    }
}

void GLGraphicsPipeline::getGLVertexFormat(VertexFormat format, GLuint& count, GLenum& glFormat)
{
    switch (format) {
        case VertexFormatUChar2:
            count = 2;
            glFormat = GL_UNSIGNED_BYTE;
            break;
        case VertexFormatChar2:
            count = 2;
            glFormat = GL_BYTE;
            break;
        case VertexFormatUChar3:
            count = 3;
            glFormat = GL_UNSIGNED_BYTE;
            break;
        case VertexFormatChar3:
            count = 3;
            glFormat = GL_BYTE;
            break;
        case VertexFormatUChar4:
            count = 4;
            glFormat = GL_UNSIGNED_BYTE;
            break;
        case VertexFormatChar4:
            count = 4;
            glFormat = GL_BYTE;
            break;
        case VertexFormatUShort2:
            count = 2;
            glFormat = GL_UNSIGNED_SHORT;
            break;
        case VertexFormatShort2:
            count = 2;
            glFormat = GL_SHORT;
            break;
        case VertexFormatUShort3:
            count = 3;
            glFormat = GL_UNSIGNED_SHORT;
            break;
        case VertexFormatShort3:
            count = 3;
            glFormat = GL_SHORT;
            break;
        case VertexFormatShort4:
            count = 4;
            glFormat = GL_SHORT;
            break;
        case VertexFormatUShort4:
            count = 4;
            glFormat = GL_UNSIGNED_SHORT;
            break;
        case VertexFormatFloat:
            count = 1;
            glFormat = GL_FLOAT;
            break;
        case VertexFormatFloat2:
            count = 2;
            glFormat = GL_FLOAT;
            break;
        case VertexFormatFloat3:
            count = 3;
            glFormat = GL_FLOAT;
            break;
        case VertexFormatFloat4:
            count = 4;
            glFormat = GL_FLOAT;
            break;
        case VertexFormatInt:
            count = 1;
            glFormat = GL_INT;
            break;
        case VertexFormatUInt:
            count = 1;
            glFormat = GL_UNSIGNED_INT;
            break;
        case VertexFormatInt2:
            count = 2;
            glFormat = GL_INT;
            break;
        case VertexFormatUInt2:
            count = 2;
            glFormat = GL_UNSIGNED_INT;
            break;
        case VertexFormatInt3:
            count = 3;
            glFormat = GL_INT;
            break;
        case VertexFormatUInt3:
            count = 3;
            glFormat = GL_UNSIGNED_INT;
            break;
        case VertexFormatInt4:
            count = 4;
            glFormat = GL_INT;
            break;
        case VertexFormatUInt4:
            count = 4;
            glFormat = GL_UNSIGNED_INT;
            break;
          
        //halfloat，这里还需要考虑不支持3.0的情况
        case VertexFormatHalfFloat:
            count = 1;
            glFormat = GL_HALF_FLOAT;
            break;
        case VertexFormatHalfFloat2:
            count = 2;
            glFormat = GL_HALF_FLOAT;
            break;
        case VertexFormatHalfFloat3:
            count = 3;
            glFormat = GL_HALF_FLOAT;
            break;
        case VertexFormatHalfFloat4:
            count = 4;
            glFormat = GL_HALF_FLOAT;
            break;
        default:
            break;
    }
}

bool GLGraphicsPipeline::getGLVertextAttribDescriptor(int index, GLVertexDescriptor& des) const
{
    bool result = false;
    //优先从在渲染管线设置的顶点属性里查
    for (auto iter = m_glVertexAttribDescrtiptor.begin(); iter != m_glVertexAttribDescrtiptor.end(); ++iter)
    {
        if (iter->index == index)
        {
            des = *iter;
            result = true;
            break;
        }
    }
    
    return result;
}

uint32_t GLGraphicsPipeline::getUBOSize(uint32_t &bindingPoint) const
{
    if (!m_glProgram)
    {
        return 0;
    }
    
    return m_glProgram->getUBOSize(bindingPoint);
}

void GLGraphicsPipeline::applyDepthStencil()
{
    if (m_glDepthDecriptor.depthCompareFunction == GL_ALWAYS)
    {
        glDisable(GL_DEPTH_TEST);
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(m_glDepthDecriptor.depthCompareFunction);
    }
    glDepthMask(m_glDepthDecriptor.depthWriteEnabled);
    
    if (!m_glStencilDescriptor.stencilEnable)
    {
        glDisable(GL_STENCIL_TEST);
    }
    else
    {
        glEnable(GL_STENCIL_TEST);
        glStencilOp(m_glStencilDescriptor.stencilFailureOperation, m_glStencilDescriptor.depthFailureOperation, m_glStencilDescriptor.depthStencilPassOperation);
    }
}

void GLGraphicsPipeline::setReferenceValue(GLuint value)
{
    if (m_glStencilDescriptor.stencilEnable)
    {
        glStencilFunc(m_glStencilDescriptor.stencilCompareFunction, value, m_glStencilDescriptor.readMask);
    }
}

NAMESPACE_RENDERCORE_END
