//
//  VKRenderDevice.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_VK_RENDER_DEVICE_INCLUDE_GFDGJ
#define GNX_ENGINE_VK_RENDER_DEVICE_INCLUDE_GFDGJ

#include "VKRenderDefine.h"
#include "RenderDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapChain.h"
#include "VKDepthStencilBuffer.h"
#include "VulkanGarbageCollector.h"
#include "VKCommandQueue.h"
#include "RenderDeviceFeatures.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderDevice : public RenderDevice
{
public:
    VKRenderDevice(ViewHandle nativeWidow);
    
    ~VKRenderDevice();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual const RenderDeviceFeatures& GetFeatures() const override
    {
        return mFeatures;
    }
    
    virtual RenderDeviceType GetRenderDeviceType() const
    {
        return RenderDeviceType::VULKAN;
    }
    
    virtual VertexBufferPtr CreateVertexBufferWithLength(uint32_t size) const;
    
    virtual VertexBufferPtr CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const;
    
    virtual IndexBufferPtr CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const;
    
    // 统一Buffer接口
    virtual RCBufferPtr CreateBuffer(const RCBufferDesc& desc) const override;
    virtual RCBufferPtr CreateBuffer(const RCBufferDesc& desc, const void* data) const override;
    
    virtual TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDesc& des) const;
    
    virtual UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const;
    
    virtual ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const;

    virtual GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const;
    
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const;
    
    virtual ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderString) const;
    
    CommandBufferPtr CreateCommandBuffer();
    
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

    void UpdateCurrentIndex();
    
    // Flush pipeline cache to disk (can be called at any time, e.g., before shutdown)
    // Does NOT destroy the Vulkan pipeline cache, just persists data to disk
    void FlushPipelineCache();

    // 从 VkPhysicalDevice 填充 RenderDeviceFeatures（在构造函数中调用一次）
    void InitializeFeatures();
    
private:
    VulkanContextPtr mVulkanContext = nullptr;
    VulkanSwapChainPtr mSwapChain = nullptr;
    
    // 结构化设备特性（在构造时从 VkPhysicalDevice 填充）
    RenderDeviceFeatures mFeatures;
    
    
    std::vector<VkCommandBuffer> mCommandBuffers;         //用于渲染的commandbuffer
    std::vector<VkCommandBuffer> mComputeCommandBuffers; //用于计算的commandbuffer
    std::vector<VkSemaphore> mImageAvailableSemaphores;   //图像可用的信号
    std::vector<VkSemaphore> mRenderFinishedSemaphores;   //渲染完成的信号
    std::vector<VkFence> mFlightFences;
    uint32_t mCurrentFrame = 0;                         //当前渲染的帧
    uint32_t mNextFrameIndex = 0;                       //可以提交命令缓冲区的交换链图像索引
    
    //创建相关的同步对象
    void CreateSyncObject();

    //销毁相关的同步对象
    void DestroySyncObject();
    
    void CreateCommandBufers(VkDevice device, size_t nImageCount, VkCommandPool commandPool);
    
    void CreateComputeCommandBuffers(VkDevice device, size_t nImageCount, VkCommandPool commandPool);
    
    void ReleaseCommandBuffers();

    // 初始化队列管理
    void InitializeCommandQueues();

    // 友元声明，允许VKCommandQueue访问私有成员
    friend class VKCommandQueue;

private:
    // 队列管理
    std::vector<VKCommandQueuePtr> mGraphicsQueues;  // 图形队列列表
    std::vector<VKCommandQueuePtr> mComputeQueues;   // 计算队列列表
    std::vector<VKCommandQueuePtr> mTransferQueues; // 传输队列列表
};

using VKRenderDevicePtr = std::shared_ptr<VKRenderDevice>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_DEVICE_INCLUDE_GFDGJ */
