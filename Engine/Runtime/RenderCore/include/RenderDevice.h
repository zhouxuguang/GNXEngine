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
#include "GraphicsPipeline.h"
#include "DeviceExtension.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "ComputeBuffer.h"
#include "RCTexture.h"
#include "CommandQueue.h"

NAMESPACE_RENDERCORE_BEGIN

class RENDERCORE_API RenderDevice
{
public:
    RenderDevice();
    
    virtual ~RenderDevice();
    
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    
    virtual DeviceExtensionPtr GetDeviceExtension() const = 0;
    
    virtual RenderDeviceType GetRenderDeviceType() const = 0;
    
    /**
     以指定长度创建buffer
     
     @param size 申请buffer长度，单位（byte）
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr CreateVertexBufferWithLength(uint32_t size) const = 0;
    
    /**
     以指定buffer和长度以内存拷贝方式创建顶点buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param mode 申请Buffer类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const = 0;
    
    virtual ComputeBufferPtr CreateComputeBuffer(uint32_t size, StorageMode mode = StorageModePrivate) const = 0;
    
    virtual ComputeBufferPtr CreateComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const = 0;
    
    /**
     以指定buffer和长度以内存拷贝方式创建索引buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param indexType 索引类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual IndexBufferPtr CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const = 0;
    
    /**
     根据采样描述创建纹理采样器

     @param des the description for sampler to be created.
     @return shared pointer to sampler object.
     */
    virtual TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDescriptor& des) const = 0;
    
    /**
     创建uniform buffer
     */
    virtual UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const = 0;
    
    /**
     创建ShaderFunctionPtr
     */
    virtual ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const = 0;

    /**
     创建GraphicsShader
     */
    virtual GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const = 0;
    
    /**
     创建图形管线
     */
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDescriptor& des) const = 0;
    
    /**
     创建计算管线
     */
    virtual ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderString) const = 0;
    
    /**
     * @brief Create a Texture2D object
     * 
     * @param format 格式
     * @param usage 用途
     * @param width 宽
     * @param height 高
     * @param levels mipmap的层数
     * @return RCTexture2DPtr 返回的2d纹理
     */
    virtual RCTexture2DPtr CreateTexture2D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const = 0;

    /**
     * @brief Create a Texture3D object
     * 
     * @param format 格式
     * @param usage 用途
     * @param width 宽
     * @param height 高
     * @param depth 深度
     * @param levels mipmap层数
     * @return RCTexture3DPtr 
     */
    virtual RCTexture3DPtr CreateTexture3D(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t depth,
                                    uint32_t levels) const = 0;

    /**
     * @brief Create a TextureCube object
     * 
     * @param format 格式
     * @param usage 用途
     * @param width 宽
     * @param height 高
     * @param levels mipmap的层数
     * @return RCTextureCubePtr 
     */
    virtual RCTextureCubePtr CreateTextureCube(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels) const = 0;

    /**
     * @brief Create a Texture2DArray object
     * 
     * @param format 格式
     * @param usage 用途
     * @param width 宽
     * @param height 高
     * @param levels mipmap的层数
     * @param arraySize 数组长度
     * @return RCTexture2DArrayPtr 
     */
    virtual RCTexture2DArrayPtr CreateTexture2DArray(TextureFormat format,
                                    TextureUsage usage,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t levels,
                                    uint32_t arraySize) const = 0;

    /**
     * @brief 根据队列类型获取队列
     *
     * @param type 队列类型（Graphics/Compute/Transfer）
     * @param index 队列索引（如果该类型有多个队列）
     * @return 返回对应类型的队列指针，失败返回nullptr
     */
    virtual CommandQueuePtr GetCommandQueue(QueueType type, uint32_t index = 0) const = 0;

    /**
     * @brief 获取指定类型的队列数量
     *
     * @param type 队列类型
     * @return 返回该类型队列的数量
     */
    virtual uint32_t GetCommandQueueCount(QueueType type) const = 0;
};

typedef std::shared_ptr<RenderDevice> RenderDevicePtr;

/**
 *    创建渲染设备
 *        @param[in] deviceType 渲染平台类型分：GLES/METAL/VULKAN
 *        @param[in] handle 操作系统本地窗口
 *        @return 渲染设备实例
 */
RENDERCORE_API RenderDevicePtr CreateRenderDevice(RenderDeviceType deviceType, ViewHandle handle);

RENDERCORE_API RenderDevicePtr GetRenderDevice();

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_DEVICE_INCLUSDGG */
