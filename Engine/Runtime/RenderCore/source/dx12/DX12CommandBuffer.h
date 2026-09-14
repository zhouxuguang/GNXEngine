//
//  DX12CommandBuffer.h
//  rendercore
//
//  D3D12 命令缓冲区。
//
//  ── 描述符环形分配（本文件最核心的设计）──
//  Vulkan 后端用 vkCmdPushDescriptorSetKHR 把描述符直接推进命令流；
//  D3D12 必须写进 shader-visible 堆再绑定表基址。这里给每个命令缓冲区
//  配一组「单帧环形堆」，按 4 类资源分别 bump 分配：
//
//    SetGraphicsPipeline → 记录本管线各表需要的宽度
//    第一次 SetXxx(...)   → 从环形堆分配一块（宽度 = 各表最大寄存器+1）
//    Draw                 → 绑定 4 张表的基址，并把当前块标记为“已消费”
//    下一次 SetXxx(...)   → 因为已消费，重新分配一块
//
//  这样两种绑定模式都正确：
//    (a) 连续多次 Draw 复用同一组绑定 → 块未被消费，继续复用；
//    (b) 每次 Draw 前重新绑定       → 块被消费，分配新块。
//  环形堆在命令缓冲区重新录制时 Reset，容量按实测留足余量（见 DX12RenderDefine.h）。
//

#ifndef GNX_ENGINE_DX12_COMMAND_BUFFER_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_COMMAND_BUFFER_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "DX12DescriptorPool.h"
#include "DX12Pipeline.h"
#include "DX12SwapChain.h"
#include "CommandBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12RenderDevice;
class DX12RenderEncoder;
class DX12ComputeEncoder;
class DX12BlitEncoder;

/// 命令缓冲区的绑定时信息
struct DX12CommandBufferInfo
{
    DX12ContextPtr context;
    DX12SwapChainPtr swapChain;          // 上屏命令缓冲区才有；离屏为 nullptr
    DX12RenderDevice* renderDevice = nullptr;
    uint32_t frameIndex = 0;             // 帧槽位下标（用于 fence 与资源多缓冲）
    uint32_t backBufferIndex = 0;
    bool isOffscreen = false;
    bool isCompute = false;
};

class DX12CommandBuffer : public CommandBuffer,
                          public std::enable_shared_from_this<DX12CommandBuffer>
{
public:
    DX12CommandBuffer(ID3D12GraphicsCommandList* commandList,
                      ID3D12CommandAllocator* commandAllocator,
                      const std::shared_ptr<DX12CommandBufferInfo>& info);

    // 注意：CommandBuffer 基类的析构函数不是虚函数（与 ComputePipeline 同样的情况）
    ~DX12CommandBuffer();

    /// 帧信息（供设备在每次复用时刷新 back buffer 索引）
    const std::shared_ptr<DX12CommandBufferInfo>& GetFrameInfo() const { return mInfo; }

    // ---- CommandBuffer 接口 ----
    RenderEncoderPtr CreateDefaultRenderEncoder(const ClearColor& clearColor) const override;
    RenderEncoderPtr CreateRenderEncoder(const RenderPass& renderPass) const override;
    ComputeEncoderPtr CreateComputeEncoder() const override;
    BlitEncoderPtr CreateBlitEncoder() const override;

    void PresentFrameBuffer() override;
    void WaitUntilCompleted() override;
    void Submit() override;

    void BeginDebugGroup(const char* name, const float color[4]) override;
    void EndDebugGroup() override;

    void ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType) override;
    void ResourceBarrier(RCBufferPtr buffer, ResourceAccessType accessType) override;

    // ---- 供 Encoder / 设备使用 ----

    ID3D12GraphicsCommandList* GetCommandList() const { return mCommandList.Get(); }
    const DX12ContextPtr& GetContext() const { return mContext; }
    const DX12SwapChainPtr& GetSwapChain() const { return mSwapChain; }
    DX12RenderDevice* GetRenderDevice() const { return mRenderDevice; }
    uint32_t GetFrameIndex() const { return mFrameIndex; }
    uint32_t GetBackBufferIndex() const { return mBackBufferIndex; }
    bool IsOffscreen() const { return mIsOffscreen; }

    /// 开始录制（重置命令分配器与描述符环形堆）
    bool BeginRecording();

    /// 结束录制（幂等）
    void EndRecording();

    /// 是否已经结束录制
    bool IsRecording() const { return mState == State::Recording; }

    /**
     * @brief 取得可写的描述符块（复用或新分配）
     *
     * 语义对齐 Vulkan 的 push descriptor：
     *  - 当前块未被任何 Draw/Dispatch 使用过 → 直接复用（继续写）；
     *  - 已被使用过 → 分配新块，并把旧块的全部描述符**拷贝继承**过来，
     *    这样“绑定持续有效直到被覆盖”的语义得以保留。
     */
    bool AcquireDescriptorBlock(DX12GraphicsPipeline* graphicsPipeline);

    /// 计算管线的描述符块分配（语义同上）
    bool AcquireComputeDescriptorBlock(DX12ComputePipeline* computePipeline);

    /// Draw / Dispatch 使用了当前块之后调用（决定下一次 Set* 是否需要新块）
    void MarkBlockUsedByDraw() { mBlockUsedByDraw = true; }

    bool HasDescriptorBlock() const { return mBlockValid; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DX12DescriptorClass cls, uint32_t registerIndex,
                                             uint32_t stageGroup = 0) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DX12DescriptorClass cls, uint32_t stageGroup = 0) const;
    uint32_t GetBlockWidth(DX12DescriptorClass cls) const;

    /// 当前描述符块的 stage 组数（1 = 普通图形/计算；3 = Mesh 的 AS/MS/PS）
    uint32_t GetStageGroupCount() const { return mBlockValid ? mBlock.groupCount : 1; }

    /// 绑定图形根签名 + 各组描述符表（Mesh 管线会绑定全部 3 组）
    void BindGraphicsDescriptorTables(bool isMeshPipeline);

    /// 绑定计算根签名 + 4 张表
    void BindComputeDescriptorTables();

    /// 当前的图形/计算根签名（由 SetPipeline 时记录）
    void SetCurrentRootSignature(ID3D12RootSignature* rootSignature, bool isMeshPipeline)
    {
        mCurrentRootSignature = rootSignature;
        mCurrentIsMeshPipeline = isMeshPipeline;
    }
    ID3D12RootSignature* GetCurrentRootSignature() const { return mCurrentRootSignature.Get(); }

    /// 当前绑定的图形管线（可能为 nullptr）
    DX12GraphicsPipeline* GetCurrentGraphicsPipeline() const { return mCurrentGraphicsPipeline; }
    void SetCurrentGraphicsPipeline(DX12GraphicsPipeline* pipeline) { mCurrentGraphicsPipeline = pipeline; }

    DX12ComputePipeline* GetCurrentComputePipeline() const { return mCurrentComputePipeline; }
    void SetCurrentComputePipeline(DX12ComputePipeline* pipeline) { mCurrentComputePipeline = pipeline; }

    /// 提交并在上屏前插入 Present 前的资源状态转换
    bool PresentToSwapChain();

private:
    enum class State
    {
        Initial,
        Recording,
        Ended,
        Submitted,
    };

    /// 描述符块的布局（必须在 InheritDescriptors 声明之前定义）
    ///
    /// 块内每个类别被划分为 groupCount 个连续段，段下标 = stage 组下标
    /// （见 DX12RenderDefine.h）。这样同一个类别的不同 stage 可以有各自的
    /// 寄存器编号，例如 Mesh 管线下 TS 的 t0 与 MS 的 t0 指向不同资源。
    struct DescriptorBlock
    {
        uint32_t base[(size_t)DX12DescriptorClass::Count]  = { 0, 0, 0, 0 };
        uint32_t width[(size_t)DX12DescriptorClass::Count] = { 1, 1, 1, 1 };

        /// stage 组数：1 = 普通图形/计算管线，3 = Mesh（AS/MS/PS）
        uint32_t groupCount = 1;

        /// 某类别在该块内占用的描述符总数（= width * groupCount）
        uint32_t Total(uint32_t cls) const { return width[cls] * groupCount; }
    };

    bool EnsureDescriptorPools();
    void BindDescriptorHeaps();

    /// 把旧块的描述符拷贝到新块（实现“绑定持续有效”的继承语义）
    void InheritDescriptors(const DescriptorBlock& oldBlock, const DescriptorBlock& newBlock);

    /// 按块宽度从「写入堆 + 绑定堆」同步分配，保证两级环的下标严格一致
    bool AllocateBlockDescriptors(DescriptorBlock& block);

    /// 把当前块的描述符从写入堆整体拷贝到 shader-visible 的绑定堆
    void SyncBlockToBindHeaps();

    /**
     * @brief 当前块的表宽是否足以覆盖该管线声明的寄存器范围
     *
     * 描述符块是按「当时那条管线」的表宽分配的。若在未发生 Draw/Dispatch 的情况下
     * 切换到另一条管线，旧块的宽度可能只覆盖 u0，而新管线要绑 u1——
     * 此时复用旧块会让绑定写入越界（GetCpuHandle 返回空句柄，
     * 最终在 CreateUnorderedAccessView 处报 id=646）。
     * 因此复用前必须校验覆盖性。
     */
    bool BlockCoversPipeline(DX12GraphicsPipeline* pipeline) const;
    bool BlockCoversPipeline(DX12ComputePipeline* pipeline) const;

    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12CommandAllocator>    mCommandAllocator;
    std::shared_ptr<DX12CommandBufferInfo> mInfo;

    DX12ContextPtr mContext;
    DX12SwapChainPtr mSwapChain;
    DX12RenderDevice* mRenderDevice = nullptr;

    uint32_t mFrameIndex = 0;
    uint32_t mBackBufferIndex = 0;
    bool mIsOffscreen = false;
    bool mIsCompute = false;

    State mState = State::Initial;

    // ---- 描述符环形堆 ----
    // 两级描述符堆。
    //
    // 写入堆（非 shader-visible）：所有视图写在这里，它同时也是描述符继承
    // （CopyDescriptors）的源。D3D12 规定 CopyDescriptors 的源不能是
    // shader-visible 堆，否则调试层报 id=654（"descriptor heap type that is
    // CPU write only, so reading it (as a copy source) is invalid"）。
    //
    // 绑定堆（shader-visible）：绑定时把当前块整体拷过来，根签名表指向这里的
    // GPU 句柄。
    std::unique_ptr<DX12DescriptorPool> mCbvSrvUavPool;
    std::unique_ptr<DX12DescriptorPool> mSamplerPool;
    std::unique_ptr<DX12DescriptorPool> mBindCbvSrvUavPool;
    std::unique_ptr<DX12DescriptorPool> mBindSamplerPool;

    DescriptorBlock mBlock;
    bool mBlockValid = false;
    bool mBlockUsedByDraw = false;   // 当前块是否已被 Draw/Dispatch 消费

    ComPtr<ID3D12RootSignature> mCurrentRootSignature;
    bool mCurrentIsMeshPipeline = false;
    DX12GraphicsPipeline* mCurrentGraphicsPipeline = nullptr;
    DX12ComputePipeline*  mCurrentComputePipeline = nullptr;

    uint32_t mDebugGroupDepth = 0;
};

using DX12CommandBufferPtr = std::shared_ptr<DX12CommandBuffer>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_COMMAND_BUFFER_INCLUDE_JHGSD */
