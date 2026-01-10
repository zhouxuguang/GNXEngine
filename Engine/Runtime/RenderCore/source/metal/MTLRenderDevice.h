//
//  MTLRenderDevice.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_MTL_RENDER_DEVICES_INCLUDE_H
#define GNX_ENGINE_MTL_RENDER_DEVICES_INCLUDE_H

#include "MTLRenderDefine.h"
#include "RenderDevice.h"
#include "MTLDeviceExtension.h"
#include "MTLCommandQueue.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLRenderDevice : public RenderDevice
{
public:
    MTLRenderDevice(CAMetalLayer *metalLayer);
    
    ~MTLRenderDevice();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual DeviceExtensionPtr GetDeviceExtension() const;
    
    virtual RenderDeviceType GetRenderDeviceType() const;
    
    /**
     以指定长度创建buffer
     
     @param size 申请buffer长度，单位（byte）
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr CreateVertexBufferWithLength(uint32_t size) const;
    
    /**
     以指定buffer和长度以内存拷贝方式创建顶点buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param mode 申请Buffer类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual VertexBufferPtr CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const;
    
    //创建计算buffer
    virtual ComputeBufferPtr CreateComputeBuffer(uint32_t size, StorageMode mode) const;
    
    virtual ComputeBufferPtr CreateComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const;
    
    /**
     以指定buffer和长度以内存拷贝方式创建索引buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param indexType 索引类型
     @return 成功申请buffer句柄，失败返回0；
     */
    virtual IndexBufferPtr CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const;
    
    /**
     根据采样描述创建纹理采样器

     @param des the description for sampler to be created.
     @return shared pointer to sampler object.
     */
    virtual TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDescriptor& des) const;
    
    /**
     创建uniform buffer
     */
    virtual UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const;
    
    /**
     创建ShaderFunctionPtr
     */
    virtual ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const;
    
    /**
    创建图形shader
     */
    virtual GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const;
    
    /**
     创建图形管线
     */
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDescriptor& des) const;
    
    /**
     创建计算管线
     */
    virtual ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderSource) const;
    
    virtual CommandBufferPtr CreateCommandBuffer();
    
    virtual CommandBufferPtr CreateComputeCommandBuffer();
    
    virtual RCTexture2DPtr CreateTexture2D(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels) const;
    
    virtual RCTexture3DPtr CreateTexture3D(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t depth,
                                        uint32_t levels) const;

    virtual RCTextureCubePtr CreateTextureCube(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels) const;

    virtual RCTexture2DArrayPtr CreateTexture2DArray(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels,
                                        uint32_t arraySize) const;

    // RHI队列接口实现
    virtual CommandQueuePtr GetCommandQueue(QueueType type, uint32_t index = 0) const override;
    virtual uint32_t GetCommandQueueCount(QueueType type) const override;

private:
    CAMetalLayer *mMetalLayer;
    id<MTLCommandQueue> mMetalCommandQueue;

    id<MTLTexture> mDepthTexture;
    id<MTLTexture> mStencilTexture;
    id<MTLTexture> mDepthStencilTexture;

    MTLDeviceExtensionPtr mMTLDeviceExtension = nullptr;

    // 队列管理（Metal的CommandQueue支持所有类型命令，这里做逻辑区分）
    std::vector<MTLCommandQueuePtr> mGraphicsQueues;  // 图形队列列表
    std::vector<MTLCommandQueuePtr> mComputeQueues;   // 计算队列列表
    std::vector<MTLCommandQueuePtr> mTransferQueues; // 传输队列列表
};

typedef std::shared_ptr<MTLRenderDevice> MTLRenderDevicePtr;
typedef std::weak_ptr<MTLRenderDevice> MTLRenderDeviceWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_DEVICES_INCLUDE_H */
