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

NAMESPACE_RENDERCORE_BEGIN

class VKRenderDevice;

struct CommandBufferInfo
{
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishSemaphore;
    VkFence flightFence;
    VulkanSwapChainPtr swapChain;
    uint32_t nextFrameIndex;
    VulkanContextPtr vulkanContext;
    VKRenderDevice *renderDevice;
};

using CommandBufferInfoPtr = std::shared_ptr<CommandBufferInfo>;

class VulkanCommandBuffer : public CommandBuffer 
{
public:
    VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferInfoPtr commandInfo);
    
    ~VulkanCommandBuffer();
    
    //创建默认的encoder，也就是屏幕渲染的encoder
    virtual RenderEncoderPtr createDefaultRenderEncoder() const;
    
    virtual RenderEncoderPtr createRenderEncoder(const RenderPass& renderPass) const;
    
    virtual ComputeEncoderPtr createComputeEncoder() const;
    
    //呈现到屏幕上，上屏
    virtual void presentFrameBuffer();
    
    //等待命令缓冲区执行完成
    virtual void waitUntilCompleted();
    
private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    CommandBufferInfoPtr mCommandInfo = nullptr;
};

using VulkanCommandBufferPtr = std::shared_ptr<VulkanCommandBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VULKAN_COMMAND_BUFFER_INCLUDE_HSDJHH */
