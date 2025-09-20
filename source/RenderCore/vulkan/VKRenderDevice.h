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

NAMESPACE_RENDERCORE_BEGIN

class VKRenderDevice : public RenderDevice
{
public:
    VKRenderDevice(ViewHandle nativeWidow);
    
    ~VKRenderDevice();
    
    virtual void Resize(uint32_t width, uint32_t height);
    
    virtual DeviceExtensionPtr GetDeviceExtension() const
    {
        return nullptr;
    }
    
    virtual RenderDeviceType GetRenderDeviceType() const
    {
        return RenderDeviceType::VULKAN;
    }
    
    virtual VertexBufferPtr CreateVertexBufferWithLength(uint32_t size) const;
    
    virtual VertexBufferPtr CreateVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const;
    
    virtual ComputeBufferPtr CreateComputeBuffer(uint32_t size) const;
    
    virtual ComputeBufferPtr CreateComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const;
    
    virtual IndexBufferPtr CreateIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const;
    
    virtual Texture2DPtr CreateTextureWithDescriptor(const TextureDescriptor& des) const;
    
    virtual TextureCubePtr CreateTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const;
    
    virtual TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDescriptor& des) const;
    
    virtual UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const;
    
    virtual ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const;

    virtual GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const;
    
    virtual GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDescriptor& des) const;
    
    virtual ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderString) const;
    
    virtual CommandBufferPtr CreateCommandBuffer();
    
    virtual RenderTexturePtr CreateRenderTexture(const TextureDescriptor& des) const;
    
    virtual RCTexture2DPtr CreateTexture2D(TextureFormat format,
                                        TextureUsage usage,
                                        uint32_t width,
                                        uint32_t height,
                                        uint32_t levels) const;
    
    void UpdateCurrentIndex()
    {
        // 更新当前帧的索引
        mCurrentFrame = (mCurrentFrame + 1) % mSwapChain->GetSwapChainImageCount();
    }
    
private:
    VulkanContextPtr mVulkanContext = nullptr;
    VulkanSwapChainPtr mSwapChain = nullptr;
    
    
    std::vector<VkCommandBuffer> mCommandBuffers;         //用于渲染的commandbuffer
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
    
    void ReleaseCommandBuffers();
};

using VKRenderDevicePtr = std::shared_ptr<VKRenderDevice>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_DEVICE_INCLUDE_GFDGJ */
