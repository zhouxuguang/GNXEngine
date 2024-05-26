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
    
    virtual void resize(uint32_t width, uint32_t height);
    
    virtual DeviceExtensionPtr getDeviceExtension() const
    {
        return nullptr;
    }
    
    virtual RenderDeviceType getRenderDeviceType() const
    {
        return RenderDeviceType::VULKAN;
    }
    
    virtual VertexBufferPtr createVertexBufferWithLength(uint32_t size) const;
    
    virtual VertexBufferPtr createVertexBufferWithBytes(const void* buffer, uint32_t size, StorageMode mode) const;
    
    virtual ComputeBufferPtr createComputeBuffer(uint32_t size) const;
    
    virtual ComputeBufferPtr createComputeBuffer(const void* buffer, uint32_t size, StorageMode mode) const;
    
    virtual IndexBufferPtr createIndexBufferWithBytes(const void* buffer, uint32_t size, IndexType indexType) const;
    
    virtual Texture2DPtr createTextureWithDescriptor(const TextureDescriptor& des) const
    {
        return nullptr;
    }
    
    virtual TextureCubePtr createTextureCubeWithDescriptor(const std::vector<TextureDescriptor>& desArray) const
    {
        return nullptr;
    }
    
    virtual TextureSamplerPtr createSamplerWithDescriptor(const SamplerDescriptor& des) const;
    
    virtual UniformBufferPtr createUniformBufferWithSize(uint32_t bufSize) const
    {
        return nullptr;
    }
    
    virtual ShaderFunctionPtr createShaderFunction(const char* pShaderSource, ShaderStage shaderStage) const
    {
        return nullptr;
    }
    
    virtual GraphicsPipelinePtr createGraphicsPipeline(const GraphicsPipelineDescriptor& des) const
    {
        return nullptr;
    }
    
    virtual ComputePipelinePtr createComputePipeline(const char* pszShaderString) const
    {
        return nullptr;
    }
    
    virtual CommandBufferPtr createCommandBuffer();
    
    virtual FrameBufferPtr createFrameBuffer(uint32_t width, uint32_t height) const
    {
        return nullptr;
    }
    
    virtual RenderTexturePtr createRenderTexture(const TextureDescriptor& des) const
    {
        return nullptr;
    }
    
    void UpdateCurrentIndex()
    {
        // 更新当前帧的索引
        mCurrentFrame = (mCurrentFrame + 1) % mSwapChain->GetSwapChainImageCount();
    }
    
private:
    VulkanContextPtr mVulkanContext = nullptr;
    VulkanSwapChainPtr mSwapChain = nullptr;
    VKDepthStencilBufferPtr mDSBuffer = nullptr;
    
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

typedef std::shared_ptr<VKRenderDevice> VKRenderDevicePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_DEVICE_INCLUDE_GFDGJ */
