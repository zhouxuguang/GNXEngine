//
//  DX12Context.h
//  rendercore
//
//  D3D12 设备上下文：DXGI 工厂 / 适配器选择 / D3D12 设备 / 内存分配器（D3D12MA）
//  / 三类命令队列 / 栅栏同步。
//
//  与 VulkanContext 的职责一一对应。
//

#ifndef GNX_ENGINE_DX12_CONTEXT_INCLUDE_JHGDSF
#define GNX_ENGINE_DX12_CONTEXT_INCLUDE_JHGDSF

#include "DX12RenderDefine.h"
#include "DX12DescriptorPool.h"
#include "RenderDeviceFeatures.h"

#include <functional>

// D3D12 Memory Allocator（AMD GPUOpen，v3.2.0）
#include "D3D12MemAlloc.h"

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// 栅栏：CPU/GPU 同步原语。D3D12 的 fence 等价于 Vulkan 的 timeline semaphore。
// ============================================================================
class DX12Fence
{
public:
    DX12Fence() = default;
    ~DX12Fence() { Destroy(); }

    // 句柄不可复制；允许移动（用于放进 std::vector 做逐帧栅栏）
    DX12Fence(const DX12Fence&) = delete;
    DX12Fence& operator=(const DX12Fence&) = delete;
    DX12Fence(DX12Fence&& other) noexcept { MoveFrom(other); }
    DX12Fence& operator=(DX12Fence&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            MoveFrom(other);
        }
        return *this;
    }

    bool Init(ID3D12Device* device, uint64_t initialValue, const char* debugName);
    void Destroy();

    bool IsValid() const { return mFence != nullptr; }

    /// 从 CPU 侧推进栅栏值（命令队列 Signal 之后调用）
    uint64_t Signal(ID3D12CommandQueue* queue);

    /// 阻塞等待直到 GPU 完成指定值
    bool WaitForValue(uint64_t value, uint32_t timeoutMs);

    /// 等待 GPU 完成当前已 Signal 的全部工作
    bool WaitForIdle(uint32_t timeoutMs);

    /// 排空命令队列：先补打一个信号再等待。
    /// Present 之后 DXGI 会在同一队列追加工作，仅 WaitForIdle() 覆盖不到，
    /// 直接 ResizeBuffers/销毁交换链会触发调试层 ERROR #921。
    bool FlushAndWait(ID3D12CommandQueue* queue, uint32_t timeoutMs)
    {
        if (queue == nullptr || mFence == nullptr)
        {
            return false;
        }
        Signal(queue);
        return WaitForIdle(timeoutMs);
    }

    uint64_t GetCompletedValue() const;
    uint64_t GetLastSignaledValue() const { return mLastSignaledValue; }

    ID3D12Fence* GetFence() const { return mFence.Get(); }

private:
    void MoveFrom(DX12Fence& other)
    {
        mFence = std::move(other.mFence);
        mEvent = other.mEvent;
        mLastSignaledValue = other.mLastSignaledValue;
        other.mEvent = nullptr;      // 避免 moved-from 析构时重复 CloseHandle
        other.mLastSignaledValue = 0;
    }

    ComPtr<ID3D12Fence> mFence;
    HANDLE mEvent = nullptr;
    uint64_t mLastSignaledValue = 0;
};

// ============================================================================
// 设备上下文
// ============================================================================
struct DX12Context
{
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter4> adapter;
    ComPtr<ID3D12Device>  device;

    // 内存分配器（AMD D3D12MA v3.2.0）：等价于 Vulkan 后端的 VMA
    D3D12MA::Allocator* allocator = nullptr;

    // ---- 命令队列 ----
    // 只创建一个 Graphics 队列（D3D12 的 DIRECT 队列同时支持图形/计算/拷贝），
    // 再按需创建独立的 Compute(Copy) 队列用于异步计算。
    ComPtr<ID3D12CommandQueue> graphicsQueue;
    ComPtr<ID3D12CommandQueue> computeQueue;
    ComPtr<ID3D12CommandQueue> copyQueue;

    // ExecuteIndirect 的命令签名由设备创建且必须活到 GPU 执行结束。
    // ByteStride 是签名的一部分，因此按调用方传入的 stride 缓存。
    std::mutex indirectSignatureMutex;
    std::unordered_map<uint32_t, ComPtr<ID3D12CommandSignature>> drawIndirectSignatures;
    std::unordered_map<uint32_t, ComPtr<ID3D12CommandSignature>> drawIndexedIndirectSignatures;
    std::unordered_map<uint32_t, ComPtr<ID3D12CommandSignature>> dispatchMeshIndirectSignatures;

    // ---- 队列栅栏 ----
    DX12Fence graphicsFence;
    DX12Fence computeFence;
    DX12Fence copyFence;

    // ---- 持久化描述符池（非 shader-visible）----
    // RTV/DSV 句柄通过 OMSetRenderTargets 直接传给命令列表，不必进 shader-visible 堆，
    // 且纹理的 RTV/DSV 在整个生命周期内不变，因此按需惰性分配并缓存。
    //
    // SRV/UAV/CBV/Sampler 则不在这里：它们在每次绑定时直接写入命令缓冲区自己的
    // 单帧环形堆（见 DX12CommandBuffer），避免跨堆拷贝与缓存失效问题。
    std::unique_ptr<DX12DescriptorPool> rtvPool;
    std::unique_ptr<DX12DescriptorPool> dsvPool;

    // ---- 设备特性缓存 ----
    // 注意：不使用 D3D12_FEATURE_DATA_D3D12_OPTIONS16（需要 SDK 10.0.26100+），
    // 以保证在 10.0.22621 上也能编译。
    D3D12_FEATURE_DATA_D3D12_OPTIONS  options  = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
    // OPTIONS18 只在较新的 D3D12 运行时上可查询；查询失败即代表运行时过旧。
    D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18 = {};

    uint32_t maxSupportedFeatureLevel = 0;   // D3D_FEATURE_LEVEL 数值
    bool     isShaderModel6_6Supported = false;
    bool     isMeshShaderSupported = false;
    bool     isRayTracingSupported = false;

    // ---- Render Pass（ID3D12GraphicsCommandList4::BeginRenderPass）----
    //
    // 依据 DirectX-Specs/d3d/RenderPasses.md：RenderPass 早期实现是有缺陷的，
    // 只有在 OPTIONS18.RenderPassesValid 为 TRUE 的运行时上使用才是定义良好的；
    // 旧运行时查询不到该 cap，此时必须回退到 OMSetRenderTargets 路径。
    D3D12_RENDER_PASS_TIER renderPassTier = D3D12_RENDER_PASS_TIER_0;
    bool isRenderPassValid = false;      // OPTIONS18.RenderPassesValid
    bool isRenderPassSupported = false;  // 设备层面的最终结论

    // 设备信息（用于填充 RenderDeviceFeatures）
    std::string deviceName;
    std::string vendorName;
    SIZE_T      dedicatedVideoMemory = 0;
    uint32_t    vendorId = 0;
    uint32_t    deviceId = 0;
    bool        isDiscreteGPU = false;

    bool IsValid() const { return device != nullptr; }

    ID3D12Device* GetDevice() const { return device.Get(); }

    /**
     * @brief 通过 D3D12MA 创建资源并命名
     *
     * @param desc 资源描述
     * @param heapType 堆类型（DEFAULT / UPLOAD / READBACK）
     * @param initialState 初始资源状态（必须与资源类型匹配，见 D3D12MA 文档）
     * @param outAllocation 输出分配对象
     * @param ppvResource 输出资源
     * @param optimizedClearValue 可空
     * @param debugName 调试名
     */
    HRESULT CreateResource(const D3D12_RESOURCE_DESC& desc,
                           D3D12_HEAP_TYPE heapType,
                           D3D12_RESOURCE_STATES initialState,
                           D3D12MA::Allocation** outAllocation,
                           REFIID riidResource,
                           void** ppvResource,
                           const D3D12_CLEAR_VALUE* optimizedClearValue,
                           const char* debugName);

    /// 释放 D3D12MA 分配（线程安全，可延迟）
    void ReleaseAllocation(D3D12MA::Allocation* allocation);

    /// 打印内存预算/用量（调试用）
    void LogMemoryStatistics();
};

using DX12ContextPtr = std::shared_ptr<DX12Context>;

// ---- 初始化步骤（与 VulkanContext 的 Create* 函数对应）----

/// 创建 DXGI 工厂（优先使用最新版本接口）
bool DX12CreateFactory(DX12Context& context);

/// 选择物理适配器（优先独显，其次任意可用设备）
bool DX12SelectAdapter(DX12Context& context);

/// 创建 D3D12 设备并查询特性
bool DX12CreateDevice(DX12Context& context);

/// 创建 D3D12MA 内存分配器
bool DX12CreateAllocator(DX12Context& context);

/// 创建命令队列与栅栏
bool DX12CreateCommandQueues(DX12Context& context);

/// 创建全局共享的 RTV/DSV 描述符池
bool DX12CreateSharedDescriptorPools(DX12Context& context);

/// 开启调试层 / DRED（必须在创建设备之前调用）
void DX12EnableDebugLayer(DX12Context& context);

/// 设备创建完成后，配置调试层的严重错误中断与 DRED 面包屑
void DX12ConfigureDebugFeatures(DX12Context& context);

/// 销毁上下文（逆序释放）
void DX12DestroyContext(DX12Context& context);

/**
 * @brief 执行一次性命令列表并等待完成（阻塞）
 *
 * 等价于 Vulkan 后端的 BeginSingleTimeCommand / EndSingleTimeCommand。
 * 用于资产上传、缓冲区初始化、回读前准备等帧外操作。
 */
bool DX12ExecuteOneShotCommandList(DX12Context& context,
                                   const std::function<void(ID3D12GraphicsCommandList*)>& recorder);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_CONTEXT_INCLUDE_JHGDSF */
