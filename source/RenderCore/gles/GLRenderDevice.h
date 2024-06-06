//
//  GLRenderDevice.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/2.
//

#ifndef GNX_ENGINE_GL_RENDERDEVICE_INCLUDE_HPP
#define GNX_ENGINE_GL_RENDERDEVICE_INCLUDE_HPP

#include "GLRenderDefine.h"
#include "RenderDevice.h"
#include "GLContextImpl.h"
#include "GLGarbgeFactory.h"
#include "GLDeviceExtension.h"
#include "GLDrawState.h"
#include "GLBinaryProgramCache.h"

NAMESPACE_RENDERCORE_BEGIN

class GLRenderDevice : public RenderDevice , public std::enable_shared_from_this<GLRenderDevice>
{
public:
    explicit GLRenderDevice(void *viewHandle);
    
    ~GLRenderDevice();
    
    virtual void resize(uint32_t width, uint32_t height);
    
    virtual DeviceExtensionPtr getDeviceExtension() const;
    
    GLDeviceExtensionPtr getGLDeviceExtension() const;
    
    virtual RenderDeviceType getRenderDeviceType() const;
    
    /**
     以指定长度创建buffer
     
     @param size 申请buffer长度，单位（byte）
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr createVertexBufferWithLength(uint32_t size) const;
    
    /**
     以指定buffer和长度以内存拷贝方式创建顶点buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param mode 申请Buffer类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const;
    
    /**
     以指定buffer和长度以内存拷贝方式创建索引buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param indexType 索引类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual IndexBufferPtr createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const;
    
    /**
     根据纹理描述创建纹理对象

     @param des the description for texture to be created
     @return shared pointer to texture object
     */
    virtual Texture2DPtr createTextureWithDescriptor(const TextureDescriptor& des) const;
    
    /**
     根据纹理描述创建立方体纹理对象

     @param desArray the description for texture to be created
     @return shared pointer to texturecube object
     */
    virtual TextureCubePtr createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const;
    
    /**
     根据采样描述创建纹理采样器

     @param des the description for sampler to be created.
     @return shared pointer to sampler object.
     */
    virtual TextureSamplerPtr createSamplerWithDescriptor(const SamplerDescriptor& des) const;
    
    /**
     创建uniform buffer
     */
    virtual UniformBufferPtr createUniformBufferWithSize(uint32_t bufSize) const;
    
    /**
     创建ShaderFunctionPtr
     */
    virtual ShaderFunctionPtr createShaderFunction(const char* pShaderSource, ShaderStage shaderStage) const;
    
    /**
     创建图形管线
     */
    virtual GraphicsPipelinePtr createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const;
    
    virtual CommandBufferPtr createCommandBuffer() const;
    
    /**
     创建renderTexture
     */
    virtual RenderTexturePtr createRenderTexture(const TextureDescriptor& des) const;
    
private:
    GLDeviceExtensionPtr m_deviceExtension = nullptr;
    GLGarbgeFactoryPtr m_glGarbgeFactory = nullptr;
    GLDrawStatePtr m_drawState = nullptr;
    GLContextImplPtr m_glContext = nullptr;
    
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

typedef std::shared_ptr<GLRenderDevice> GLRenderDevicePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_RENDERDEVICE_INCLUDE_HPP */
