//
//  VKRenderEncoder.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG
#define GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG

#include "VulkanContext.h"
#include "RenderEncoder.h"
#include "VKGraphicsPipeline.h"
#include "VulkanRenderPass.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderEncoder : public RenderEncoder
{
public:
    VKRenderEncoder(VulkanContextPtr context, 
                    VkCommandBuffer commandBuffer,
                    const VkRenderingInfoKHR& renderInfo,
                    const RenderPassFormat& passFormat, 
                    const RenderPassImage& passImage,
                    const std::vector<VkClearValue> &clearValues,
                    const RenderPassImageView& passImageView,
                    uint32_t currentFrameIndex);
    
    ~VKRenderEncoder();
    
    virtual void EndEncode();
    
    virtual void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline);
    
    virtual void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index);
    
    virtual void SetVertexUniformBuffer(UniformBufferPtr buffer, int index);
    
    virtual void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index);

    virtual void SetFragmentUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer);

    virtual void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture);

    virtual void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);

    virtual void SetVertexUAVBuffer(const std::string& resourceName, ComputeBufferPtr buffer);

    virtual void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);
    
    virtual void DrawPrimitves(PrimitiveMode mode, int offset, int size);

    virtual void DrawInstancePrimitves(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount);
    
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset);

    virtual void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
        uint32_t firstInstance, uint32_t instanceCount);

    virtual void DrawPrimitvesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);

    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, ComputeBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);
    
    virtual void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler);
    
private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VKGraphicsPipeline *mGraphicsPipieline = nullptr;
    VulkanRenderPassPtr mRenderPass = nullptr;
    VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;
    RenderPassFormat mPassFormat;
    RenderPassImage mPassImage;
    VulkanContextPtr mContext = nullptr;
    uint32_t mCurrentFrameIndex = 0;
    
    void BeginDynamicRenderPass(const VkRenderingInfoKHR& renderInfo);
    void EndDynamicRenderPass();
    
    void BeginRenderPass(const VkRenderingInfoKHR& renderInfo, 
                         const RenderPassFormat& passFormat,
                         const RenderPassImage& passImage, 
                         const std::vector<VkClearValue> &clearValues,
                         const RenderPassImageView& passImageView);
    void EndRenderPass();
    
    void BindPipeline();
};

using VKRenderEncoderPtr = std::shared_ptr<VKRenderEncoder>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG */
