//
//  GLRenderEncoder.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/4/30.
//

#include "GLRenderEncoder.h"
#include "GLVertexBuffer.h"
#include "GLUniformBuffer.h"
#include "GLIndexBuffer.h"
#include "GLTexture2D.h"
#include "GLTextureCube.h"
#include "GLTextureSampler.h"
#include "GLRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

#define BUFFER_OFFSET(i) ((unsigned char *)NULL + (i))

GLRenderEncoder::GLRenderEncoder(GLDrawStatePtr drawState)
{
    m_currentState = drawState;
}

GLRenderEncoder::~GLRenderEncoder()
{
    //
}

void GLRenderEncoder::endEncoder()
{
    //
}

/**
 设置图形管线
 */
void GLRenderEncoder::setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
{
    if (!graphicsPipeline)
    {
        return;
    }
    assert(m_currentState);
    
    GLGraphicsPipelinePtr glPipeline = std::dynamic_pointer_cast<GLGraphicsPipeline>(graphicsPipeline);
    if (m_currentState->pipeline != nullptr)
    {
        //相同就不做事
        if (m_currentState->pipeline == glPipeline)
        {
            return;
        }
        m_currentState->pipeline->unBind();
        m_currentState->pipeline = glPipeline;
    }
    else
    {
        m_currentState->pipeline = glPipeline;
    }
    glPipeline->apply();
}

/**
 Description
 
 @param buffer buffer对象
 @param index 绑定的索引
 */
void GLRenderEncoder::setVertexBuffer(VertexBufferPtr buffer, int index)
{
    if (nullptr == buffer)
    {
        return;
    }
    assert(m_currentState);

    GLGraphicsPipelinePtr pGLPileline = (m_currentState->pipeline);
    if (!pGLPileline)
    {
        return;
    }

    //先查找应用层传过来的属性
    GLVertexDescriptor verAttrib;
    bool ret = pGLPileline->getGLVertextAttribDescriptor(index, verAttrib);
    if (!ret)
    {
        return;
    }

    GLVertexBufferPtr glbuffer = std::dynamic_pointer_cast<GLVertexBuffer>(buffer);
    bool isVBO = glbuffer->isVBO();
    if (isVBO)
    {
        glbuffer->Bind();
        pGLPileline->enableAttribute(index);
        glVertexAttribPointer(index, verAttrib.size, verAttrib.type, verAttrib.normalized, verAttrib.stride, BUFFER_OFFSET(verAttrib.offset));
    }
    else
    {
        glbuffer->UnBind();
        uint8_t *data = (uint8_t*)glbuffer->mapBufferData();
        pGLPileline->enableAttribute(index);
        glVertexAttribPointer(index, verAttrib.size, verAttrib.type, verAttrib.normalized, verAttrib.stride, data + verAttrib.offset);
        glbuffer->unmapBufferData(data);
    }
}

/**
 设置顶点数据，以copy的方式直接设置，pData的大小dataLen最大为4096，即4K
 
 @param pData 数据指针
 @param dataLen 数据长度
 @param index 绑定的索引
 */
void GLRenderEncoder::setVertexBytes(const void* pData, size_t dataLen, int index)
{
    if (!pData || dataLen <= 0)
    {
        return;
    }
    
    GLGraphicsPipelinePtr pGLPileline = (m_currentState->pipeline);
    if (!pGLPileline)
    {
        return;
    }

    //先查找应用层传过来的属性
    GLVertexDescriptor verAttrib;
    bool ret = pGLPileline->getGLVertextAttribDescriptor(index, verAttrib);
    if (!ret)
    {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    pGLPileline->enableAttribute(index);
    glVertexAttribPointer(index, verAttrib.size, verAttrib.type, verAttrib.normalized, verAttrib.stride, pData);
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void GLRenderEncoder::setVertexUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }
    
    if (!m_currentState->pipeline)
    {
        return;
    }
    
    uint32_t bindIndex = index;
    int dataSize = m_currentState->pipeline->getUBOSize(bindIndex);
    
    GLUniformBufferPtr glBuffer = std::dynamic_pointer_cast<GLUniformBuffer>(buffer);
    glBuffer->apply(dataSize, bindIndex);
}

/**
 设置uniformbuffer的索引
 
 @param buffer buffer description
 @param index index description
 */
void GLRenderEncoder::setFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    if (!buffer)
    {
        return;
    }
    
    if (!m_currentState->pipeline)
    {
        return;
    }
    
    GLUniformBufferPtr glBuffer = std::dynamic_pointer_cast<GLUniformBuffer>(buffer);
    uint32_t bindingIndex = index;
    glBuffer->apply(m_currentState->pipeline->getUBOSize(bindingIndex), index);
}

static GLuint getGLPrimtiveMode(PrimitiveMode mode)
{
    GLuint glmode = 0;
    switch (mode)
    {
        case PrimitiveMode_POINTS:
            glmode = GL_POINTS;
            break;
        case PrimitiveMode_LINES:
            glmode = GL_LINES;
            break;
        case PrimitiveMode_LINE_STRIP:
            glmode = GL_LINE_STRIP;
            break;
        case PrimitiveMode_TRIANGLES:
            glmode = GL_TRIANGLES;
            break;
        case PrimitiveMode_TRIANGLE_STRIP:
            glmode = GL_TRIANGLE_STRIP;
            break;
        default:
            break;
    }
    return glmode;
}

/**
 draw function
 
 @param mode mode description
 @param offset offset description
 @param size size description
 */
void GLRenderEncoder::drawPrimitves(PrimitiveMode mode, int offset, int size)
{
    GLenum glmode = getGLPrimtiveMode(mode);
    GLCALL(glDrawArrays(glmode, offset, size));
}

/**
 draw funton with index
 
 @param mode mode description
 @param size size description
 @param buffer buffer description
 @param offset offset description
 */
void GLRenderEncoder::drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset)
{
    if (buffer == nullptr)
    {
        return;
    }
    
    std::shared_ptr<GLIndexBuffer> glBuffer = std::dynamic_pointer_cast<GLIndexBuffer>(buffer);
    if (!glBuffer)
    {
        return;
    }
    
    //先绑定IBO对象，上传数据
    GLenum glmode = getGLPrimtiveMode(mode);
    
    IndexType indexType = glBuffer->getIndexType();
    
    int byteOffset = offset * sizeof(uint16_t);
    GLenum dataType = GL_UNSIGNED_SHORT;
    if (indexType == IndexType_UInt)
    {
        byteOffset = offset * sizeof(uint32_t);
        dataType = GL_UNSIGNED_INT;
    }
    
    if (glBuffer->isEBO())
    {
        glBuffer->Bind();
        glDrawElements(glmode, size, dataType, BUFFER_OFFSET(byteOffset));
    }

    else
    {
        void* indexBuffer = glBuffer->mapBufferData();
        if (indexBuffer == nullptr)
        {
            return;
        }
        
        glBuffer->UnBind();
        glDrawElements(glmode, size, dataType, (uint8_t*)indexBuffer + byteOffset);
    }
}

/**
 设置纹理和采样器

 @param texture 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void GLRenderEncoder::setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index)
{
    bool returnFlag = false;
    if (texture == nullptr)
    {
        GLTexture2D::unbind(index);
        returnFlag = true;
    }
    
    if (sampler == nullptr)
    {
        GLTextureSampler::unbind(index);
        returnFlag = true;
    }
    
    if (returnFlag)
    {
        return;
    }
    
    std::dynamic_pointer_cast<GLTexture2D>(texture)->apply(index);
    std::dynamic_pointer_cast<GLTextureSampler>(sampler)->apply(index);
}

/**
 设置立方体纹理和采样器

 @param textureCube 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void GLRenderEncoder::setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index)
{
    bool returnFlag = false;
    if (textureCube == nullptr)
    {
        GLTextureCube::unbind(index);
        returnFlag = true;
    }
    
    if (sampler == nullptr)
    {
        GLTextureSampler::unbind(index);
        returnFlag = true;
    }
    
    if (returnFlag)
    {
        return;
    }
    
    std::dynamic_pointer_cast<GLTextureCube>(textureCube)->apply(index);
    std::dynamic_pointer_cast<GLTextureSampler>(sampler)->apply(index);
}

/**
 设置渲染纹理和采样器

 @param renderTexture 纹理句柄
 @param sampler 采样器句柄
 @param index 纹理通道索引
 */
void GLRenderEncoder::setFragmentRenderTextureAndSampler(RenderTexturePtr renderTexture, TextureSamplerPtr sampler, int index)
{
    bool returnFlag = false;
    if (renderTexture == nullptr)
    {
        GLTexture2D::unbind(index);    //这里和texture是一样的
        returnFlag = true;
    }
    
    if (sampler == nullptr)
    {
        GLTextureSampler::unbind(index);
        returnFlag = true;
    }
    
    if (returnFlag)
    {
        return;
    }
    
    std::dynamic_pointer_cast<GLRenderTexture>(renderTexture)->apply(index);
    std::dynamic_pointer_cast<GLTextureSampler>(sampler)->apply(index);
}

NAMESPACE_RENDERCORE_END
