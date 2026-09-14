//
//  DX12RenderEncoder.h
//  rendercore
//
//  D3D12 图形渲染编码器。
//
//  ── 与 Vulkan 后端的语义对齐 ──
//  Vulkan 用 vkCmdPushDescriptorSetKHR + VK_EXT_extended_dynamic_state，
//  D3D12 下分别映射为：
//    描述符  → 命令缓冲区的单帧环形堆（见 DX12CommandBuffer.h）
//    动态状态 → PSO 变体（fillMode / depthBias）+ 命令列表动态状态
//              （primitive topology、scissor、stencil reference）
//
//  ── RenderPass 的模拟 ──
//  D3D12 没有 render pass 对象。这里把 RenderPass 的 load/store 语义展开为：
//    loadOp=CLEAR  → ClearRenderTargetView / ClearDepthStencilView
//    loadOp=LOAD   → 仅插入状态转换
//    进入时把附件转换到 RENDER_TARGET / DEPTH_WRITE（只读深度用 DEPTH_READ），
//    退出时转到 SHADER_READ（离屏）或 PRESENT（上屏）。
//

#ifndef GNX_ENGINE_DX12_RENDER_ENCODER_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_RENDER_ENCODER_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12CommandBuffer.h"
#include "DX12Pipeline.h"
#include "RenderEncoder.h"
#include "RenderPass.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12RenderEncoder : public RenderEncoder
{
public:
    /// 使用 RenderPass（含附件 load/store 语义）
    DX12RenderEncoder(const DX12CommandBufferPtr& commandBuffer, const RenderPass& renderPass);

    /// 默认上屏编码器（清屏颜色 + 交换链深度缓冲）
    DX12RenderEncoder(const DX12CommandBufferPtr& commandBuffer,
                      const ClearColor& clearColor,
                      bool useSwapChainTargets);

    ~DX12RenderEncoder() override;

    void EndEncode() override;

    void SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline) override;
    void SetFillMode(FillMode fillMode) override;

    void SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index) override;
    void SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index) override;

    void SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage) override;

    void DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
                                uint32_t drawCount, uint32_t stride) override;
    void DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
                                       int indexBufferOffset, RCBufferPtr indirectBuffer,
                                       uint32_t indirectBufferOffset,
                                       uint32_t drawCount, uint32_t stride) override;
    void DrawIndexedPrimitivesIndirectCount(PrimitiveMode mode, IndexBufferPtr indexBuffer,
                                            int indexBufferOffset, RCBufferPtr indirectBuffer,
                                            uint32_t indirectBufferOffset,
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
    void DrawInstancePrimitives(PrimitiveMode mode, int offset, int size,
                                uint32_t firstInstance, uint32_t instanceCount) override;
    void DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer,
                               int offset, int baseVertex) override;
    void DrawIndexedInstancePrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer,
                                       int offset, uint32_t firstInstance,
                                       uint32_t instanceCount) override;

    void SetFragmentTextureAndSampler(const std::string& resourceName, RCTexturePtr texture,
                                      TextureSamplerPtr sampler) override;
    void SetVertexTextureAndSampler(const std::string& resourceName, RCTexturePtr texture,
                                    TextureSamplerPtr sampler) override;
    void SetMeshTextureAndSampler(const std::string& resourceName, RCTexturePtr texture,
                                  TextureSamplerPtr sampler) override;
    void SetTaskTextureAndSampler(const std::string& resourceName, RCTexturePtr texture,
                                  TextureSamplerPtr sampler) override;

    void DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                               uint32_t drawCount, uint32_t stride) override;

    void SetScissorRect(int x, int y, uint32_t width, uint32_t height) override;
    void SetDepthBias(float bias, float slopeScale, float clamp) override;
    void SetStencilReference(uint32_t frontRef, uint32_t backRef) override;

private:
    /// 渲染目标描述（由 RenderPass 或交换链展开而来）
    struct TargetInfo
    {
        std::vector<std::shared_ptr<DX12TextureBase>> colorTextures;
        std::shared_ptr<DX12TextureBase> depthTexture;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> colorRTVs;
        D3D12_CPU_DESCRIPTOR_HANDLE depthDSV = {};
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> colorClearRTVs;   // 需要 clear 的附件
        std::vector<std::array<float, 4>> colorClearColors;        // 与 colorClearRTVs 一一对应
        bool hasDepth = false;
        bool depthReadOnly = false;
        bool clearDepth = false;
        float clearDepthValue = 1.0f;
        uint32_t clearStencilValue = 0;
        bool hasStencil = false;
        uint32_t width = 0;
        uint32_t height = 0;
        DX12RenderPassFormat psoFormat;
    };

    void BuildTargetsFromRenderPass(const RenderPass& renderPass);
    void BuildSwapChainTargets(const ClearColor& clearColor);
    void BeginTargets();
    void EndTargets();

    /// 在附件与 PSO 格式变化时确保 PSO 存在并绑定
    void BindPipelineIfNeeded();

    DX12PSOVariantKey CurrentVariantKey() const
    {
        DX12PSOVariantKey key;
        key.fillMode = mCurrentFillMode;
        key.depthBiasFactor = mDepthBiasSlope;
        key.depthBiasUnits = mDepthBiasUnits;
        return key;
    }

    // ---- 绑定辅助 ----
    /// 确保当前描述符块可用（必要时重新分配）
    bool EnsureBlock();

    /**
     * @brief 按资源名解析出绑定信息（类别 + 寄存器号）
     *
     * Mesh 管线下必须传 stage：各 stage 的 HLSL 寄存器独立分配，
     * 合并结果（stage = nullptr）只代表第一个使用该资源的 stage。
     */
    const DX12BindInfo* ResolveBinding(const std::string& name,
                                       const ShaderStage* stage = nullptr) const;

    /// ShaderStage → 描述符块的 stage 组（非 Mesh 管线恒为 0）
    uint32_t StageGroup(ShaderStage stage) const;

    /// 把资源写入当前块的对应寄存器槽位（stageGroup 见 DX12RenderDefine.h）
    void WriteCBV(uint32_t registerIndex, UniformBufferPtr buffer, uint32_t stageGroup = 0);
    void WriteSRVTexture(uint32_t registerIndex, RCTexturePtr texture, uint32_t stageGroup = 0);
    void WriteUAVTexture(uint32_t registerIndex, RCTexturePtr texture, uint32_t stageGroup = 0);
    void WriteSRVBuffer(uint32_t registerIndex, RCBufferPtr buffer, bool asUAV,
                        bool isRawBuffer, uint32_t structuredStride, uint32_t stageGroup = 0);
    void WriteSampler(uint32_t registerIndex, TextureSamplerPtr sampler, uint32_t stageGroup = 0);

    void BindIndexBuffer(IndexBufferPtr buffer, int indexOffset);
    void ApplyTopology(PrimitiveMode mode);

    /// Draw / Dispatch 前统一提交描述符表绑定
    void FlushDescriptorTables();

    DX12CommandBufferPtr mCommandBuffer;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    DX12Context* mContext = nullptr;

    DX12GraphicsPipeline* mGraphicsPipeline = nullptr;
    FillMode mCurrentFillMode = FillModeSolid;
    float mDepthBiasUnits = 0.0f;
    float mDepthBiasSlope = 0.0f;

    TargetInfo mTargets;
    bool mIsSwapChainPass = false;
    bool mEncoding = false;
};

using DX12RenderEncoderPtr = std::shared_ptr<DX12RenderEncoder>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_RENDER_ENCODER_INCLUDE_JHGSD */
