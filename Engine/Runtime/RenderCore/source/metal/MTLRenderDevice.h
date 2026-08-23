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
#include "MTLCommandQueue.h"
#include "RenderDeviceFeatures.h"
#include "MTLPipelineCache.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLRenderDevice : public RenderDevice
{
public:
    MTLRenderDevice(CAMetalLayer *metalLayer);
    
    ~MTLRenderDevice();
    
    void Resize(uint32_t width, uint32_t height) override;
    
    RenderDeviceType GetRenderDeviceType() const override;
    
    virtual const RenderDeviceFeatures& GetFeatures() const override
    {
        return mFeatures;
    }
    
    /**
     以指定长度创建buffer
     
     @param size 申请buffer长度，单位（byte）
     @return 成功申请buffer句柄，失败返回0；
     */
    VertexBufferPtr CreateVertexBufferWithLength(uint32_t size) const override;
    
    /**
     以指定buffer和长度以内存拷贝方式创建顶点buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param mode 申请Buffer类型
     @return 成功申请buffer句柄，失败返回0；
     */
    VertexBufferPtr CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const override;
    
    /**
     以指定buffer和长度以内存拷贝方式创建索引buffer
     
     @param buffer 指定buffer内容
     @param size buffer长度
     @param indexType 索引类型
     @return 成功申请buffer句柄，失败返回0；
     */
    IndexBufferPtr CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const override;
    
    // 统一Buffer接口
    virtual RCBufferPtr CreateBuffer(const RCBufferDesc& desc) const override;
    virtual RCBufferPtr CreateBuffer(const RCBufferDesc& desc, const void* data) const override;
    
    /**
     根据采样描述创建纹理采样器

     @param des the description for sampler to be created.
     @return shared pointer to sampler object.
     */
    TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDesc& des) const override;
    
    /**
     创建uniform buffer
     */
    UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const override;
    
    /**
     创建ShaderFunctionPtr
     */
    ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const override;
    
    /**
    创建图形shader
     */
    GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const override;
    
    /**
    创建Mesh图形shader (Task + Mesh + Fragment)
     */
    GraphicsShaderPtr CreateMeshGraphicsShader(const ShaderCode& taskShader, const ShaderCode& meshShader, const ShaderCode& fragmentShader) const override;
    
    /**
     创建图形管线
     */
    GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const override;
    
    /**
     创建计算管线
     */
    ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderSource) const override;
    
    CommandBufferPtr CreateCommandBuffer();
    
    RCTexture2DPtr CreateTexture2D(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels) const override;
    
    RCTexture3DPtr CreateTexture3D(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t depth,
                                        uint32_t levels) const override;

    RCTextureCubePtr CreateTextureCube(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels) const override;

    RCTexture2DArrayPtr CreateTexture2DArray(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels,
                                        uint32_t arraySize) const override;

    // RHI队列接口实现
    virtual CommandQueuePtr GetCommandQueue(QueueType type, uint32_t index = 0) const override;
    virtual uint32_t GetCommandQueueCount(QueueType type) const override;

    // 友元声明，允许MTLCommandQueue访问私有成员
    friend class MTLCommandQueue;

    // 从 MTLDevice 填充 RenderDeviceFeatures（在构造函数中调用一次）
    void InitializeFeatures();

    // Pipeline Cache (Metal Binary Archive)
    void InitializePipelineCache();
    void ShutdownPipelineCache();
    MTLPipelineCachePtr GetPipelineCache() const { return mPipelineCache; }

    void SetVSync(bool enable) override;
    bool IsVSync() const override;

private:
    CAMetalLayer *mMetalLayer;
    id<MTLCommandQueue> mMetalCommandQueue;
    bool mVSync = true;

    id<MTLTexture> mDepthTexture;
    id<MTLTexture> mStencilTexture;
    id<MTLTexture> mDepthStencilTexture;

    // 结构化设备特性（在构造时从 MTLDevice 填充）
    RenderDeviceFeatures mFeatures;

    // 队列管理（Metal的CommandQueue支持所有类型命令，这里做逻辑区分）
    std::vector<MTLCommandQueuePtr> mGraphicsQueues;  // 图形队列列表
    std::vector<MTLCommandQueuePtr> mComputeQueues;   // 计算队列列表
    std::vector<MTLCommandQueuePtr> mTransferQueues; // 传输队列列表

    // Metal Binary Archive Pipeline Cache
    MTLPipelineCachePtr mPipelineCache;
};

typedef std::shared_ptr<MTLRenderDevice> MTLRenderDevicePtr;
typedef std::weak_ptr<MTLRenderDevice> MTLRenderDeviceWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_RENDER_DEVICES_INCLUDE_H */
