//
//  VulkanCommandBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/24.
//

#ifndef GNX_ENGINE_VULKAN_COMMAND_BUFFER_INCLUDE_HSDJHH
#define GNX_ENGINE_VULKAN_COMMAND_BUFFER_INCLUDE_HSDJHH

#include "VulkanContext.h"
#include "CommandBuffer.h"
#include "VulkanSwapChain.h"
#include "VKDepthStencilBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderDevice;

struct CommandBufferInfo
{
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishSemaphore;
    VkFence flightFence;
    VulkanSwapChainPtr swapChain;
    uint32_t currentFrameIndex;
    uint32_t nextFrameIndex;
    VulkanContextPtr vulkanContext;
    VKDepthStencilBufferPtr depthStencilBuffer;
    VKRenderDevice *renderDevice;
    bool isComputeCommandBuffer = false;  // 是否为计算命令缓冲区
};

using CommandBufferInfoPtr = std::shared_ptr<CommandBufferInfo>;

class VulkanCommandBuffer : public CommandBuffer 
{
public:
    VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferInfoPtr commandInfo);
    
    ~VulkanCommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    virtual RenderEncoderPtr CreateDefaultRenderEncoder() const;
    
    virtual RenderEncoderPtr CreateRenderEncoder(const RenderPass& renderPass) const;
    
    virtual ComputeEncoderPtr CreateComputeEncoder() const;
    
    virtual BlitEncoderPtr CreateBlitEncoder() const;
    
    //呈现到屏幕上，上屏
    virtual void PresentFrameBuffer();
    
    //等待命令缓冲区执行完成
    virtual void WaitUntilCompleted();
    
    //提交命令缓冲区（用于计算命令缓冲区）
    virtual void Submit();
    
    // 开始调试标记
    virtual void BeginDebugGroup(const char* name, const float color[4]);
    
    // 结束调试标记
    virtual void EndDebugGroup();

    /**
     * 通知命令缓冲区纹理资源即将被访问
     * 自动处理Vulkan的layout转换
     * @param texture 纹理资源
     * @param accessType 访问类型（读/写、着色器读取、颜色附件等）
     */
    virtual void ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType) override;

    /**
     * 通知命令缓冲区统一缓冲区资源即将被访问
     * 自动处理Vulkan的buffer barrier
     * @param buffer 统一缓冲区资源
     * @param accessType 访问类型（读/写、着色器读取等）
     */
    virtual void ResourceBarrier(RCBufferPtr buffer, ResourceAccessType accessType) override;

private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    CommandBufferInfoPtr mCommandInfo = nullptr;
};

using VulkanCommandBufferPtr = std::shared_ptr<VulkanCommandBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_COMMAND_BUFFER_INCLUDE_HSDJHH */
