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
    
    void EndEncode() override;
    
    void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline) override;
    
    void SetFillMode(FillMode fillMode) override;
    
    void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index) override;
    
    // RCBuffer接口
    void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index) override;
    
    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage) override;
    
    void DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
        uint32_t drawCount, uint32_t stride) override;

    void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
        int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
        uint32_t drawCount, uint32_t stride) override;

    void DrawIndexedPrimitivesIndirectCount(PrimitiveMode mode, IndexBufferPtr indexBuffer,
        int indexBufferOffset, RCBufferPtr indirectBuffer, uint32_t indirectBufferOffset,
        RCBufferPtr countBuffer, uint32_t countBufferOffset,
        uint32_t maxDrawCount, uint32_t stride) override;
    
    void SetVertexUniformBuffer(UniformBufferPtr buffer, int index) override;
    
    void SetFragmentUniformBuffer(UniformBufferPtr buffer, int index) override;

    void SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture) override;

    void SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    void SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    void SetMeshUniformBuffer(UniformBufferPtr buffer, int index) override;

    void SetTaskUniformBuffer(UniformBufferPtr buffer, int index) override;

    void SetMeshUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    void SetTaskUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) override;

    void DrawPrimitives(PrimitiveMode mode, int offset, int size) override;

    void DrawInstancePrimitives(PrimitiveMode mode, int offset, int size, uint32_t firstInstance, uint32_t instanceCount) override;
    
    void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset, int baseVertex = 0) override;

    void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset,
        uint32_t firstInstance, uint32_t instanceCount) override;
    
    void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetMeshTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;
    void SetTaskTextureAndSampler(const std::string& resourceName, RCTexturePtr texture, TextureSamplerPtr sampler) override;

    // ===== Mesh Shader 绘制接口 =====
    void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                       uint32_t drawCount, uint32_t stride) override;

    // ===== 动态渲染状态接口 =====
    void SetScissorRect(int x, int y, uint32_t width, uint32_t height) override;
    void SetDepthBias(float bias, float slopeScale, float clamp) override;
    void SetStencilReference(uint32_t frontRef, uint32_t backRef) override;
    
private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VKGraphicsPipeline *mGraphicsPipieline = nullptr;
    FillMode mCurrentFillMode = FillModeSolid;   // 当前填充模式（驱动不支持动态 polygonMode 时用于选择 PSO 变体）
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
