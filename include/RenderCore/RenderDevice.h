//
//  RenderDevice.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/1.
//

#ifndef GNX_ENGINE_RENDER_DEVICE_INCLUSDGG
#define GNX_ENGINE_RENDER_DEVICE_INCLUSDGG

#include "RenderDefine.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "TextureCube.h"
#include "GraphicsPipeline.h"
#include "DeviceExtension.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "RenderTexture.h"
#include "ComputeBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class RenderDevice
{
public:
    RenderDevice();
    
    virtual ~RenderDevice();
    
    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    virtual DeviceExtensionPtr getDeviceExtension() const = 0;
    
    virtual RenderDeviceType getRenderDeviceType() const = 0;
    
    /**
     以指定长度创建buffer
     
     @param size 申请buffer长度，单位（byte）
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr createVertexBufferWithLength(uint32_t size) const = 0;
    
    /**
     以指定buffer和长度以内存拷贝方式创建顶点buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param mode 申请Buffer类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const = 0;
    
    virtual ComputeBufferPtr createComputeBuffer(uint32_t size) const = 0;
    
    virtual ComputeBufferPtr createComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const = 0;
    
    /**
     以指定buffer和长度以内存拷贝方式创建索引buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param indexType 索引类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual IndexBufferPtr createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const = 0;
    
    /**
     根据纹理描述创建纹理对象

     @param des the description for texture to be created
     @return shared pointer to texture object
     */
    virtual Texture2DPtr createTextureWithDescriptor(const TextureDescriptor& des) const = 0;
    
    /**
     根据纹理描述创建立方体纹理对象

     @param desArray the description for texture to be created
     @return shared pointer to texturecube object
     */
    virtual TextureCubePtr createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const = 0;
    
    /**
     根据采样描述创建纹理采样器

     @param des the description for sampler to be created.
     @return shared pointer to sampler object.
     */
    virtual TextureSamplerPtr createSamplerWithDescriptor(const SamplerDescriptor& des) const = 0;
    
    /**
     创建uniform buffer
     */
    virtual UniformBufferPtr createUniformBufferWithSize(uint32_t bufSize) const = 0;
    
    /**
     创建ShaderFunctionPtr
     */
    virtual ShaderFunctionPtr createShaderFunction(const char* pShaderSource, ShaderStage shaderStage) const = 0;
    
    /**
     创建图形管线
     */
    virtual GraphicsPipelinePtr createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const = 0;
    
    /**
     创建计算管线
     */
    virtual ComputePipelinePtr createComputePipeline(const char* pszShaderString) const = 0;
    
    virtual CommandBufferPtr createCommandBuffer() = 0;
    
    /**
     创建FrameBuffer
     */
    virtual FrameBufferPtr createFrameBuffer(uint32_t width, uint32_t height) const = 0;
    
    /**
     创建renderTexture
     */
    virtual RenderTexturePtr createRenderTexture(const TextureDescriptor& des) const = 0;
};

typedef std::shared_ptr<RenderDevice> RenderDevicePtr;

/**
 *    创建渲染引擎实例
 *        @param[in] deviceType 渲染平台类型分：GL/METAL/VULKAN
 *        @return 渲染引擎实例
 */
RenderDevicePtr createRenderDevice(RenderDeviceType deviceType, ViewHandle handle);

RenderDevicePtr getRenderDevice();

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_DEVICE_INCLUSDGG */
