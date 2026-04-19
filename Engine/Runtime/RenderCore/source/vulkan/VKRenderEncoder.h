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
#include "VKRCBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderEncoder : public RenderEncoder
{
public:
    VKRenderEncoder(VulkanContextPtr context,
                    VkCommandBuffer commandBuffer,
                    const VkRenderingInfoKHR& renderInfo,
                    const RenderPassFormat& passFormat,
                    const RenderPassImage& passImage,
                    const RenderPassTexture& passTexture,
                    const std::vector<VkClearValue> &clearValues,
                    const RenderPassImageView& passImageView,
                    uint32_t currentFrameIndex);
    
    ~VKRenderEncoder();
    
    virtual void EndEncode();
    
    virtual void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline);
    
    virtual void SetFillMode(FillMode fillMode);
    
    virtual void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index);
    
    // RCBuffer接口
    virtual void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index);
    
    virtual void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage);
    
    virtual void DrawPrimitvesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);

    virtual void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride);
    
    virtual void SetVertexUniformBuffer(UniformBufferPtr buffer, int index);
    
    virtual void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index);

    virtual void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture);

    virtual void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);

    virtual void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer);
    
    virtual void DrawPrimitves(PrimitiveMode mode, int offset, int size);

    virtual void DrawInstancePrimitves(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount);
    
    virtual void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex = 0);

    virtual void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
        uint32_t firstInstance, uint32_t instanceCount);
    
    virtual void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler);
    
    // ===== Mesh Shader 绘制接口 =====
    virtual void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    virtual void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                       uint32_t drawCount, uint32_t stride);
    
private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VKGraphicsPipeline *mGraphicsPipieline = nullptr;
    VulkanRenderPassPtr mRenderPass = nullptr;
    VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;
    RenderPassFormat mPassFormat;
    RenderPassImage mPassImage;
    RenderPassTexture mPassTexture;
    VulkanContextPtr mContext = nullptr;
    uint32_t mCurrentFrameIndex = 0;
    bool mIsEncoding = true;  // 跟踪 render pass 是否活跃，析构时自动结束
    
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
