//
//  GLRenderDevice.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/2.
//

#include "GLRenderDevice.h"
#include "GLVertexBuffer.h"
#include "GLIndexBuffer.h"
#include "GLTexture2D.h"
#include "GLTextureCube.h"
#include "GLTextureSampler.h"
#include "GLUniformBuffer.h"
#include "GLShaderFunction.h"
#include "gl3stub.h"
#include "GLCommandBuffer.h"
#include "GLFrameBuffer.h"
#include "GLShaderFunction.h"
#include "GLRenderTexture.h"

NAMESPACE_RENDERCORE_BEGIN

static void MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
   printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR_KHR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

GLRenderDevice::GLRenderDevice(void *viewHandle)
{
    //初始化扩展函数
    gleswInit();
    gl3stubInit();
    
    // During init, enable debug output
    if (glDebugMessageCallbackKHR)
    {
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glDebugMessageCallbackKHR(MessageCallback, 0);
    }
    
//    GLuint program_pipeline;
//    glGenProgramPipelinesEXT(1, &program_pipeline);
//    glDeleteProgramPipelinesEXT(1, &program_pipeline);
    
#ifdef __APPLE__
    m_glContext = createEAGLContext(viewHandle);
#else
#endif
    m_glContext->setFramebuffer();
    
    std::make_shared<GLBinaryProgramCache>();
    
    //m_renderContext = std::make_shared<GLRenderContext>(viewHandle, width, height);
    OpenGLESContext::initCurrentContext();
    m_glGarbgeFactory = std::make_shared<GLGarbgeFactory>();
    m_deviceExtension = std::make_shared<GLDeviceExtension>();
    m_deviceExtension->GatherGPUInfo();
    
    m_drawState = std::make_shared<GLDrawState>();
}

GLRenderDevice::~GLRenderDevice()
{
    m_deviceExtension.reset();
}

void GLRenderDevice::resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
}

DeviceExtensionPtr GLRenderDevice::getDeviceExtension() const
{
    return m_deviceExtension;
}

GLDeviceExtensionPtr GLRenderDevice::getGLDeviceExtension() const
{
    return m_deviceExtension;
}

RenderDeviceType GLRenderDevice::getRenderDeviceType() const
{
    return RenderDeviceType::GLES;
}

/**
 以指定长度创建buffer
 
 @param size 申请buffer长度，单位（byte）
 @return 成功申请buffer句柄，失败返回0；
 */
VertexBufferPtr GLRenderDevice::createVertexBufferWithLength(uint32_t size) const
{
    auto vertexBuffer = std::make_shared<GLVertexBuffer>(size, StorageModeShared);
    return vertexBuffer;
}

/**
 以指定buffer和长度以内存拷贝方式创建顶点buffer
 
 @param buffer 指定buffer内容
 @param size buffer长度
 @param mode 申请Buffer类型
 @return 成功申请buffer句柄，失败返回0；
 */
VertexBufferPtr GLRenderDevice::createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const
{
    auto vertexBuffer = std::make_shared<GLVertexBuffer>(buffer, size, mode);
    return vertexBuffer;
}

/**
 以指定buffer和长度以内存拷贝方式创建索引buffer
 
 @param buffer 指定buffer内容
 @param size buffer长度
 @param indexType 索引类型
 @return 成功申请buffer句柄，失败返回0；
 */
IndexBufferPtr GLRenderDevice::createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const
{
    auto indexBuffer = std::make_shared<GLIndexBuffer>(indexType, buffer, size);
    return indexBuffer;
}

/**
 根据纹理描述创建纹理对象

 @param des the description for texture to be created
 @return shared pointer to texture object
 */
Texture2DPtr GLRenderDevice::createTextureWithDescriptor(const TextureDescriptor& des) const
{
    auto texture2d = std::make_shared<GLTexture2D>(des);
    return texture2d;
}

TextureCubePtr GLRenderDevice::createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const
{
    return std::make_shared<GLTextureCube>(desArray);
}

/**
 根据采样描述创建纹理采样器

 @param des the description for sampler to be created.
 @return shared pointer to sampler object.
 */
TextureSamplerPtr GLRenderDevice::createSamplerWithDescriptor(const SamplerDescriptor& des) const
{
    GLGarbgeFactoryWeakPtr garbgeFactoryWeakPtr = m_glGarbgeFactory;
    auto textureSampler = std::make_shared<GLTextureSampler>(garbgeFactoryWeakPtr, des);
    return textureSampler;
}

/**
 创建uniform buffer
 */
UniformBufferPtr GLRenderDevice::createUniformBufferWithSize(uint32_t bufSize) const
{
    auto uniformBuffer = std::make_shared<GLUniformBuffer>(bufSize);
    return uniformBuffer;
}

ShaderFunctionPtr GLRenderDevice::createShaderFunction(const char* pShaderSource, ShaderStage shaderStage) const
{
    GLShaderFunctionPtr shaderFunction = std::make_shared<GLShaderFunction>(shared_from_this());
    return shaderFunction->initWithShaderSource(pShaderSource, shaderStage);
}

GraphicsPipelinePtr GLRenderDevice::createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const
{
    return std::make_shared<GLGraphicsPipeline>(des);
}

CommandBufferPtr GLRenderDevice::createCommandBuffer() const
{
    return std::make_shared<GLCommandBuffer>(m_drawState, m_glContext, m_width, m_height);
}

FrameBufferPtr GLRenderDevice::createFrameBuffer(uint32_t width, uint32_t height) const
{
    return std::make_shared<GLFrameBuffer>(width, height);
}

RenderTexturePtr GLRenderDevice::createRenderTexture(const TextureDescriptor& des) const
{
    return std::make_shared<GLRenderTexture>(des);
}

NAMESPACE_RENDERCORE_END
