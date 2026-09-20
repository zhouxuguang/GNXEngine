//
//  DX12RenderDevice.h
//  rendercore
//
//  D3D12 渲染设备：RHI 的最终接线层。对应 Vulkan 后端的 VKRenderDevice。
//

#ifndef GNX_ENGINE_DX12_RENDER_DEVICE_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_RENDER_DEVICE_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "DX12SwapChain.h"
#include "DX12RootSignature.h"
#include "DX12CommandQueue.h"
#include "DX12CommandBuffer.h"
#include "RenderDevice.h"
#include "RenderDeviceFeatures.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12RenderDevice : public RenderDevice
{
public:
    explicit DX12RenderDevice(const NativeWindow& nativeWindow);
    ~DX12RenderDevice() override;

    void Resize(uint32_t width, uint32_t height) override;
    void OnWindowRestored(const NativeWindow& nativeWindow) override;
    void OnWindowMinimized() override;

    const RenderDeviceFeatures& GetFeatures() const override { return mFeatures; }

    RenderDeviceType GetRenderDeviceType() const override { return RenderDeviceType::DX12; }

    RCBufferPtr CreateBuffer(const RCBufferDesc& desc) const override;
    RCBufferPtr CreateBuffer(const RCBufferDesc& desc, const void* data) const override;

    TextureSamplerPtr CreateSamplerWithDescriptor(const SamplerDesc& des) const override;
    UniformBufferPtr CreateUniformBufferWithSize(uint32_t bufSize) const override;

    ShaderFunctionPtr CreateShaderFunction(const ShaderCode& shaderSource, ShaderStage shaderStage) const override;
    GraphicsShaderPtr CreateGraphicsShader(const ShaderCode& vertexShader, const ShaderCode& fragmentShader) const override;
    GraphicsShaderPtr CreateGraphicsShader(const ShaderStageData& vertexShader,
                                           const ShaderStageData& fragmentShader) const override;
    GraphicsShaderPtr CreateMeshGraphicsShader(const ShaderCode& taskShader, const ShaderCode& meshShader,
                                               const ShaderCode& fragmentShader) const override;
    GraphicsShaderPtr CreateMeshGraphicsShader(const ShaderStageData& taskShader,
                                               const ShaderStageData& meshShader,
                                               const ShaderStageData& fragmentShader) const override;

    GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const override;
    ComputePipelinePtr CreateComputePipeline(const ShaderCode& shaderString) const override;
    ComputePipelinePtr CreateComputePipeline(const ShaderStageData& shader) const override;

    RCTexture2DPtr CreateTexture2D(TextureFormat format, TextureUsage usage,
                                   uint32_t width, uint32_t height, uint32_t levels) const override;
    RCTexture3DPtr CreateTexture3D(TextureFormat format, TextureUsage usage,
                                   uint32_t width, uint32_t height, uint32_t depth,
                                   uint32_t levels) const override;
    RCTextureCubePtr CreateTextureCube(TextureFormat format, TextureUsage usage,
                                       uint32_t width, uint32_t height, uint32_t levels) const override;
    RCTexture2DArrayPtr CreateTexture2DArray(TextureFormat format, TextureUsage usage,
                                             uint32_t width, uint32_t height, uint32_t levels,
                                             uint32_t arraySize) const override;

    CommandQueuePtr GetCommandQueue(QueueType type, uint32_t index = 0) const override;
    uint32_t GetCommandQueueCount(QueueType type) const override;

    void SetVSync(bool enable) override;
    bool IsVSync() const override;

    void FlushPipelineCache() override;

    // ---- 供命令队列 / 命令缓冲区使用 ----

    CommandBufferPtr CreateCommandBuffer();

    const DX12ContextPtr& GetContext() const { return mContext; }
    DX12SwapChain* GetSwapChain() const { return mSwapChain.get(); }
    DX12RootSignature* GetRootSignature() const { return mRootSignature.get(); }

    /// 每帧的槽位状态（主渲染路径使用）
    uint32_t GetCurrentFrameIndex() const { return mCurrentFrameIndex; }
    uint32_t GetBackBufferIndex() const { return mBackBufferIndex; }
    void AdvanceFrameIndex();

    /// 帧槽位对应的命令分配器与命令列表
    ID3D12CommandAllocator* GetFrameCommandAllocator(uint32_t frameIndex) const
    {
        return (frameIndex < mFrameCommandAllocators.size())
                   ? mFrameCommandAllocators[frameIndex].Get() : nullptr;
    }
    ID3D12GraphicsCommandList* GetFrameCommandList(uint32_t frameIndex) const
    {
        return (frameIndex < mFrameCommandLists.size())
                   ? mFrameCommandLists[frameIndex].Get() : nullptr;
    }

    /// 等待指定帧槽位的 GPU 工作完成（返回 false 表示超时）
    bool WaitForFrameSlot(uint32_t frameIndex, uint32_t timeoutMs);

    /**
     * @brief 在图形队列上推进并 signal 指定帧槽位的栅栏
     *
     * 必须由命令缓冲区在 ExecuteCommandLists 之后调用。否则该槽位的命令分配器
     * 会在 GPU 仍在使用时被下一帧 Reset，触发设备移除。
     */
    void SignalFrameFence(uint32_t frameIndex);

    /// 该帧槽位是否正在被 GPU 使用
    bool IsFrameSlotInFlight(uint32_t frameIndex) const;

    /// 记录在帧槽位上交由 GPU 完成后的回调（用于延迟释放）
    void EnqueueFrameDeferredRelease(uint32_t frameIndex, std::function<void()>&& release);

    void InitializeFeatures();

private:
    bool Initialize(const NativeWindow& nativeWindow);
    void CreateFrameResources();
    void ReleaseFrameResources();

    DX12ContextPtr mContext;
    DX12SwapChainPtr mSwapChain;
    std::unique_ptr<DX12RootSignature> mRootSignature;
    RenderDeviceFeatures mFeatures;

    // 队列（与 context 中的三条队列一一对应）
    std::vector<DX12CommandQueuePtr> mGraphicsQueues;
    std::vector<DX12CommandQueuePtr> mComputeQueues;
    std::vector<DX12CommandQueuePtr> mTransferQueues;

    // 帧槽位资源（多帧在飞）
    std::vector<ComPtr<ID3D12CommandAllocator>> mFrameCommandAllocators;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> mFrameCommandLists;
    std::vector<DX12Fence> mFrameFences;                  // 每个帧槽位的完成栅栏
    std::vector<std::vector<std::function<void()>>> mFrameDeferredReleases;

    // 缓存的命令缓冲区（每个帧槽位一个），复用其描述符堆避免每帧重建
    std::vector<DX12CommandBufferPtr> mFrameCommandBuffers;

    uint32_t mCurrentFrameIndex = 0;
    uint32_t mBackBufferIndex = 0;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    bool mInitialized = false;
    bool mVSync = true;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_RENDER_DEVICE_INCLUDE_JHGSD */
