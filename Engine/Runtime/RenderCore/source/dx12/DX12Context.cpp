//
//  DX12Context.cpp
//  rendercore
//

#include "DX12Context.h"
#include "DX12Helpers.h"

#include <d3d12sdklayers.h>

#include <cstdio>
#include <cstdlib>

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// 调试层：消息回调
// ============================================================================
#if GNX_DX12_DEBUG_LAYER
namespace
{
void WINAPI DX12MessageCallback(D3D12_MESSAGE_CATEGORY category,
                                D3D12_MESSAGE_SEVERITY severity,
                                D3D12_MESSAGE_ID id,
                                LPCSTR description,
                                void* context)
{
    switch (severity)
    {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
            LOG_ERROR("[DX12][CORRUPTION] id=%d cat=%d : %s", (int)id, (int)category, description);
            break;
        case D3D12_MESSAGE_SEVERITY_ERROR:
            LOG_ERROR("[DX12][ERROR] id=%d cat=%d : %s", (int)id, (int)category, description);
            break;
        case D3D12_MESSAGE_SEVERITY_WARNING:
            LOG_WARN("[DX12][WARN] id=%d cat=%d : %s", (int)id, (int)category, description);
            break;
        case D3D12_MESSAGE_SEVERITY_INFO:
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            LOG_DEBUG("[DX12][INFO] id=%d cat=%d : %s", (int)id, (int)category, description);
            break;
        default: break;
    }

    (void)context;
}
} // namespace
#endif

// ============================================================================
// DX12Fence
// ============================================================================

bool DX12Fence::Init(ID3D12Device* device, uint64_t initialValue, const char* debugName)
{
    if (device == nullptr)
    {
        return false;
    }

    HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] CreateFence failed for '%s': %s", debugName ? debugName : "?",
                  DX12HResultToString(hr));
        return false;
    }

    mLastSignaledValue = initialValue;

    mEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (mEvent == nullptr)
    {
        LOG_ERROR("[DX12] CreateEventEx failed for fence '%s'", debugName ? debugName : "?");
        mFence.Reset();
        return false;
    }

    if (debugName)
    {
        std::wstring wideName(debugName, debugName + strlen(debugName));
        mFence->SetName(wideName.c_str());
    }

    return true;
}

void DX12Fence::Destroy()
{
    if (mEvent != nullptr)
    {
        CloseHandle(mEvent);
        mEvent = nullptr;
    }
    mFence.Reset();
    mLastSignaledValue = 0;
}

uint64_t DX12Fence::Signal(ID3D12CommandQueue* queue)
{
    if (queue == nullptr || mFence == nullptr)
    {
        return mLastSignaledValue;
    }
    ++mLastSignaledValue;
    const HRESULT hr = queue->Signal(mFence.Get(), mLastSignaledValue);
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] ID3D12CommandQueue::Signal failed: %s", DX12HResultToString(hr));
    }
    return mLastSignaledValue;
}

bool DX12Fence::WaitForValue(uint64_t value, uint32_t timeoutMs)
{
    if (mFence == nullptr || mEvent == nullptr)
    {
        return false;
    }

    // 已经完成
    if (mFence->GetCompletedValue() >= value)
    {
        return true;
    }

    const HRESULT hr = mFence->SetEventOnCompletion(value, mEvent);
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] SetEventOnCompletion failed (value=%llu): %s",
                  (unsigned long long)value, DX12HResultToString(hr));
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(mEvent, timeoutMs == 0 ? INFINITE : (DWORD)timeoutMs);
    if (waitResult != WAIT_OBJECT_0)
    {
        LOG_ERROR("[DX12] Fence wait timeout after %u ms (value=%llu, completed=%llu)",
                  timeoutMs, (unsigned long long)value,
                  (unsigned long long)mFence->GetCompletedValue());
        return false;
    }
    return true;
}

bool DX12Fence::WaitForIdle(uint32_t timeoutMs)
{
    return WaitForValue(mLastSignaledValue, timeoutMs);
}

uint64_t DX12Fence::GetCompletedValue() const
{
    return (mFence != nullptr) ? (uint64_t)mFence->GetCompletedValue() : 0;
}

// ============================================================================
// 调试层 / DRED 配置
// ============================================================================

void DX12EnableDebugLayer(DX12Context& context)
{
#if GNX_DX12_DEBUG_LAYER
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))) && debugController)
    {
        debugController->EnableDebugLayer();
        LOG_INFO("[DX12] Debug layer enabled.");

#if GNX_DX12_GPU_BASED_VALIDATION
        ComPtr<ID3D12Debug1> debug1;
        if (SUCCEEDED(debugController.As(&debug1)) && debug1)
        {
            debug1->SetEnableGPUBasedValidation(TRUE);
            LOG_INFO("[DX12] GPU-based validation enabled (expect a large slowdown).");
        }
#endif
    }
    else
    {
        LOG_WARN("[DX12] Debug layer requested but D3D12GetDebugInterface failed. "
                 "Install 'Graphics Tools' optional feature to enable it.");
    }
#endif // GNX_DX12_DEBUG_LAYER

#if GNX_DX12_ENABLE_DRED
    // DRED 必须在设备创建之前配置才会生效
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))) && dredSettings)
    {
        dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        LOG_INFO("[DX12] DRED (auto breadcrumbs + page fault) enabled.");
    }
#endif
}

void DX12ConfigureDebugFeatures(DX12Context& context)
{
    if (!context.IsValid())
    {
        return;
    }

#if GNX_DX12_DEBUG_LAYER
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(context.device.As(&infoQueue)) && infoQueue)
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

        // 过滤掉驱动/系统固有的噪声消息（与渲染正确性无关）
        D3D12_MESSAGE_ID hide[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        infoQueue->AddStorageFilterEntries(&filter);

        // 注册回调，让错误即时出现在日志里（而不是退出时统一 dump）
        ComPtr<ID3D12InfoQueue1> infoQueue1;
        if (SUCCEEDED(context.device.As(&infoQueue1)) && infoQueue1)
        {
            DWORD cookie = 0;
            infoQueue1->RegisterMessageCallback(&DX12MessageCallback,
                                                D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie);
        }
    }
#endif
}

// ============================================================================
// 工厂 / 适配器 / 设备
// ============================================================================

bool DX12CreateFactory(DX12Context& context)
{
    UINT flags = 0;
#if GNX_DX12_DEBUG_LAYER
    flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&context.factory));
    if (FAILED(hr))
    {
        // 调试标志在某些环境下会失败（例如未安装图形工具），退回普通工厂
        hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&context.factory));
    }

    if (FAILED(hr) || !context.factory)
    {
        LOG_ERROR("[DX12] CreateDXGIFactory2 failed: %s", DX12HResultToString(hr));
        return false;
    }

    return true;
}

bool DX12SelectAdapter(DX12Context& context)
{
    if (!context.factory)
    {
        return false;
    }

    ComPtr<IDXGIAdapter4> bestAdapter;
    SIZE_T bestVideoMemory = 0;

    // 优先枚举高性能（独显）适配器
    ComPtr<IDXGIAdapter1> adapter1;
    for (UINT i = 0;
         context.factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                    IID_PPV_ARGS(&adapter1)) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        if (!adapter1)
        {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc1 = {};
        if (FAILED(adapter1->GetDesc1(&desc1)))
        {
            adapter1.Reset();
            continue;
        }

        // 跳过软件适配器（WARP / Basic Render Driver）
        // 注意：如果系统没有硬件 GPU，会保留最后一个可用适配器作为兜底。
        const bool isSoftware = (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

        // 试探能否创建 D3D12 设备
        if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0,
                                        __uuidof(ID3D12Device), nullptr)) && !isSoftware)
        {
            if (desc1.DedicatedVideoMemory > bestVideoMemory)
            {
                bestVideoMemory = desc1.DedicatedVideoMemory;
                bestAdapter.Reset();
                adapter1.As(&bestAdapter);
            }
        }

        adapter1.Reset();
    }

    // 没找到硬件适配器：回退到第一个能创建 D3D12 设备的适配器（可能是 WARP）
    if (!bestAdapter)
    {
        LOG_WARN("[DX12] No hardware adapter found; falling back to the first D3D12-capable adapter.");
        for (UINT i = 0;
             context.factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND;
             ++i)
        {
            if (!adapter1)
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0,
                                            __uuidof(ID3D12Device), nullptr)))
            {
                adapter1.As(&bestAdapter);
                adapter1.Reset();
                break;
            }
            adapter1.Reset();
        }
    }

    if (!bestAdapter)
    {
        LOG_ERROR("[DX12] Failed to find any D3D12-capable adapter.");
        return false;
    }

    context.adapter = bestAdapter;

    DXGI_ADAPTER_DESC1 desc1 = {};
    if (SUCCEEDED(context.adapter->GetDesc1(&desc1)))
    {
        // 宽字符设备名转 UTF-8（用 Windows API 避免手写转换）
        char nameUtf8[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, desc1.Description, -1, nameUtf8, sizeof(nameUtf8) - 1,
                            nullptr, nullptr);
        context.deviceName          = nameUtf8;
        context.vendorId            = desc1.VendorId;
        context.deviceId            = desc1.DeviceId;
        context.dedicatedVideoMemory = desc1.DedicatedVideoMemory;
        context.isDiscreteGPU       = (desc1.DedicatedVideoMemory > 0) &&
                                      ((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0);

        switch (desc1.VendorId)
        {
            case 0x10DE: context.vendorName = "NVIDIA";  break;
            case 0x1002:
            case 0x1022: context.vendorName = "AMD";     break;
            case 0x8086: context.vendorName = "Intel";   break;
            case 0x13B5: context.vendorName = "ARM";     break;
            case 0x1414: context.vendorName = "Microsoft"; break;
            default:     context.vendorName = "Unknown"; break;
        }

        LOG_INFO("[DX12] Adapter: %s (%s), dedicated VRAM = %llu MB",
                 context.deviceName.c_str(), context.vendorName.c_str(),
                 (unsigned long long)(desc1.DedicatedVideoMemory / (1024 * 1024)));
    }

    return true;
}

bool DX12CreateDevice(DX12Context& context)
{
    if (!context.adapter)
    {
        return false;
    }

    // 从高到低尝试特性等级
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL createdLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = E_FAIL;
    for (D3D_FEATURE_LEVEL level : featureLevels)
    {
        hr = D3D12CreateDevice(context.adapter.Get(), level, IID_PPV_ARGS(&context.device));
        if (SUCCEEDED(hr) && context.device)
        {
            createdLevel = level;
            break;
        }
        context.device.Reset();
    }

    if (!context.device)
    {
        LOG_ERROR("[DX12] D3D12CreateDevice failed for all feature levels: %s",
                  DX12HResultToString(hr));
        return false;
    }

    context.maxSupportedFeatureLevel = (uint32_t)createdLevel;
    LOG_INFO("[DX12] Device created, feature level = 0x%04X", (unsigned)createdLevel);

    // ---- 查询设备特性 ----
    auto query = [&](D3D12_FEATURE feature, void* data, size_t size) -> bool {
        return SUCCEEDED(context.device->CheckFeatureSupport(feature, data, (UINT)size));
    };

    query(D3D12_FEATURE_D3D12_OPTIONS,   &context.options,   sizeof(context.options));
    query(D3D12_FEATURE_D3D12_OPTIONS1,  &context.options1,  sizeof(context.options1));
    query(D3D12_FEATURE_D3D12_OPTIONS5,  &context.options5,  sizeof(context.options5));
    query(D3D12_FEATURE_D3D12_OPTIONS7,  &context.options7,  sizeof(context.options7));
    query(D3D12_FEATURE_D3D12_OPTIONS12, &context.options12, sizeof(context.options12));

    // ---- Render Pass ----
    // RenderPassesTier 只表示驱动对 RenderPass 的"利用程度"：
    //   TIER_0 = 驱动未实现 DDI 表，由运行时翻译成等价的 OMSetRenderTargets
    //   TIER_1 = UMD 实现，RT/DB 写入可加速
    //   TIER_2 = TIER_1 + pass 内 UAV 写入高效
    // tier 本身不影响合法性 —— 真正的门槛是 OPTIONS18.RenderPassesValid。
    // （实测 NVIDIA RTX 5060：RenderPassesValid=1 但 tier=0，此时调用
    //   BeginRenderPass 完全合法，只是由运行时做翻译。）
    context.renderPassTier = context.options5.RenderPassesTier;

    // RenderPassesValid 决定"调用 RenderPass API 是否定义良好"。旧运行时查询会失败。
    bool options18Queried = query(D3D12_FEATURE_D3D12_OPTIONS18, &context.options18,
                                  sizeof(context.options18));
    context.isRenderPassValid = options18Queried && (context.options18.RenderPassesValid != FALSE);
    context.isRenderPassSupported = context.isRenderPassValid;

    // ---- Shader Model 支持（用于判断 SM 6.6 = bindless 资源）----
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
    shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
    if (query(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
    {
        context.isShaderModel6_6Supported = (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6);
    }

    // ---- Mesh Shader ----
    context.isMeshShaderSupported = (context.options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);

    // ---- 光线追踪 ----
    context.isRayTracingSupported =
        (context.options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);

    LOG_INFO("[DX12] MeshShader=%d (tier %d), RayTracing=%d, ShaderModel6.6=%d, WaveOps=%d",
             (int)context.isMeshShaderSupported, (int)context.options7.MeshShaderTier,
             (int)context.isRayTracingSupported, (int)context.isShaderModel6_6Supported,
             (int)context.options1.WaveOps);

    LOG_INFO("[DX12] RenderPass: tier=%d, RenderPassesValid(cap queried)=%d, deviceSupported=%d",
             (int)context.renderPassTier, (int)context.isRenderPassValid,
             (int)context.isRenderPassSupported);

    DX12ConfigureDebugFeatures(context);
    return true;
}

bool DX12CreateAllocator(DX12Context& context)
{
    if (!context.device || !context.adapter)
    {
        return false;
    }

    D3D12MA::ALLOCATOR_DESC desc = {};
    desc.pDevice  = context.device.Get();
    desc.pAdapter = context.adapter.Get();
    // 不能使用 D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS：它包含
    // ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED / _ALLOW_..._NOT_ZEROED，会让
    // D3D12MA 用 D3D12_HEAP_FLAG_CREATE_NOT_ZEROED 创建堆。而 D3D12 禁止
    // 带 RenderTarget / DepthStencil 标志的资源使用该堆标志，会在
    // ExecuteCommandLists 时报错（id=1422）并可能导致设备移除。
    desc.Flags    = D3D12MA::ALLOCATOR_FLAG_NONE;
    // 0 = 使用库默认值（当前为 64 MiB）
    desc.PreferredBlockSize = 0;
    desc.pAllocationCallbacks = nullptr;

    const HRESULT hr = D3D12MA::CreateAllocator(&desc, &context.allocator);
    if (FAILED(hr) || context.allocator == nullptr)
    {
        LOG_ERROR("[DX12] D3D12MA::CreateAllocator failed: %s", DX12HResultToString(hr));
        return false;
    }

    LOG_INFO("[DX12] D3D12MA allocator created (block size = %llu MB, UMA=%d, tightAlign=%d)",
             (unsigned long long)(64ull),
             (int)context.allocator->IsUMA(),
             (int)context.allocator->IsTightAlignmentSupported());
    return true;
}

bool DX12CreateCommandQueues(DX12Context& context)
{
    if (!context.device)
    {
        return false;
    }

    auto createQueue = [&](D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority,
                           const wchar_t* name, ComPtr<ID3D12CommandQueue>& outQueue) -> bool {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type     = type;
        desc.Priority = (INT)priority;
        desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        const HRESULT hr = context.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&outQueue));
        if (FAILED(hr) || !outQueue)
        {
            LOG_ERROR("[DX12] CreateCommandQueue failed (type=%d): %s", (int)type,
                      DX12HResultToString(hr));
            return false;
        }
        outQueue->SetName(name);
        return true;
    };

    if (!createQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                     L"DX12 Graphics Queue", context.graphicsQueue))
    {
        return false;
    }

    // 计算/拷贝队列是可选优化项：失败不影响主流程（D3D12 的 DIRECT 队列本就能做计算与拷贝）
    if (createQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                    L"DX12 Compute Queue", context.computeQueue))
    {
        context.computeFence.Init(context.device.Get(), 0, "DX12 Compute Fence");
    }

    if (createQueue(D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                    L"DX12 Copy Queue", context.copyQueue))
    {
        context.copyFence.Init(context.device.Get(), 0, "DX12 Copy Fence");
    }

    if (!context.graphicsFence.Init(context.device.Get(), 0, "DX12 Graphics Fence"))
    {
        return false;
    }

    return true;
}

bool DX12CreateSharedDescriptorPools(DX12Context& context)
{
    if (!context.device)
    {
        return false;
    }

    // RTV/DSV 数量按需增长；这里给一个足够大的初始容量（每类 4096），
    // 若不足由 DX12Texture 在分配失败时给出明确错误日志。
    constexpr uint32_t kRTVCapacity = 4096;
    constexpr uint32_t kDSVCapacity = 1024;

    context.rtvPool = std::make_unique<DX12DescriptorPool>();
    if (!context.rtvPool->Init(context.device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                               kRTVCapacity, false, "DX12 RTV Pool"))
    {
        return false;
    }

    context.dsvPool = std::make_unique<DX12DescriptorPool>();
    if (!context.dsvPool->Init(context.device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                               kDSVCapacity, false, "DX12 DSV Pool"))
    {
        return false;
    }

    return true;
}

// ============================================================================
// 资源创建
// ============================================================================

HRESULT DX12Context::CreateResource(const D3D12_RESOURCE_DESC& desc,
                                    D3D12_HEAP_TYPE heapType,
                                    D3D12_RESOURCE_STATES initialState,
                                    D3D12MA::Allocation** outAllocation,
                                    REFIID riidResource,
                                    void** ppvResource,
                                    const D3D12_CLEAR_VALUE* optimizedClearValue,
                                    const char* debugName)
{
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = heapType;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
    allocDesc.CustomPool = nullptr;
    allocDesc.pPrivateData = nullptr;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

    // 带 RenderTarget / DepthStencil 标志的资源使用独立（committed）分配。
    //
    // 原因：D3D12MA 默认把资源放在共享堆里做成 placed 资源，而 D3D12 要求
    // 带 RT/DS 标志的 placed 资源在首次使用前必须先被初始化（Discard/Clear/Copy），
    // 否则调试层在 ExecuteCommandLists 时报 id=1422
    // （"Placed resources ... must be initialized with a Discard/Clear/Copy"）。
    // committed 资源由运行时保证内容已定义，不存在该前置约束。
    //
    // D3D12MA 文档也明确建议对这类资源（典型是全屏 RT）使用 ALLOCATION_FLAG_COMMITTED。
    if ((desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                       D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0)
    {
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
    }

    return allocator->CreateResource(&allocDesc, &desc, initialState, optimizedClearValue,
                                     outAllocation, riidResource, ppvResource);
}

void DX12Context::ReleaseAllocation(D3D12MA::Allocation* allocation)
{
    if (allocation != nullptr)
    {
        allocation->Release();
    }
}

void DX12Context::LogMemoryStatistics()
{
    if (allocator == nullptr || adapter == nullptr)
    {
        return;
    }

    ComPtr<IDXGIAdapter3> adapter3;
    if (FAILED(adapter.As(&adapter3)) || !adapter3)
    {
        return;
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO localInfo = {};
    DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo = {};
    if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo)))
    {
        LOG_INFO("[DX12][Memory] local: budget=%llu MB, usage=%llu MB",
                 (unsigned long long)(localInfo.Budget / (1024 * 1024)),
                 (unsigned long long)(localInfo.CurrentUsage / (1024 * 1024)));
    }
    if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo)))
    {
        LOG_INFO("[DX12][Memory] non-local: budget=%llu MB, usage=%llu MB",
                 (unsigned long long)(nonLocalInfo.Budget / (1024 * 1024)),
                 (unsigned long long)(nonLocalInfo.CurrentUsage / (1024 * 1024)));
    }

    D3D12MA::TotalStatistics stats = {};
    allocator->CalculateStatistics(&stats);
    const D3D12MA::Statistics& total = stats.Total.Stats;
    LOG_INFO("[DX12][Memory] D3D12MA: blocks=%u (%llu MB), allocations=%u (%llu MB)",
             (unsigned)total.BlockCount,
             (unsigned long long)(total.BlockBytes / (1024 * 1024)),
             (unsigned)total.AllocationCount,
             (unsigned long long)(total.AllocationBytes / (1024 * 1024)));
}

bool DX12ExecuteOneShotCommandList(DX12Context& context,
                                   const std::function<void(ID3D12GraphicsCommandList*)>& recorder)
{
    if (!context.IsValid() || !context.graphicsQueue)
    {
        return false;
    }

    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    if (FAILED(context.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(&allocator))) ||
        FAILED(context.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 allocator.Get(), nullptr,
                                                 IID_PPV_ARGS(&commandList))))
    {
        LOG_ERROR("[DX12] DX12ExecuteOneShotCommandList: failed to create command list");
        return false;
    }

    recorder(commandList.Get());
    commandList->Close();

    ID3D12CommandList* lists[] = { commandList.Get() };
    context.graphicsQueue->ExecuteCommandLists(1, lists);
    context.graphicsFence.Signal(context.graphicsQueue.Get());

    if (!context.graphicsFence.WaitForIdle(15000))
    {
        LOG_ERROR("[DX12] DX12ExecuteOneShotCommandList: GPU wait timed out");
        return false;
    }

    return true;
}

void DX12DestroyContext(DX12Context& context)
{
    // 顺序很重要：先让 GPU 空闲，再按 队列 → 分配器 → 设备 → 工厂 逆序释放
    if (context.graphicsFence.IsValid())
    {
        context.graphicsFence.FlushAndWait(context.graphicsQueue.Get(), 5000);
    }
    if (context.computeFence.IsValid() && context.computeQueue)
    {
        context.computeFence.FlushAndWait(context.computeQueue.Get(), 5000);
    }
    if (context.copyFence.IsValid() && context.copyQueue)
    {
        context.copyFence.FlushAndWait(context.copyQueue.Get(), 5000);
    }

    context.graphicsFence.Destroy();
    context.computeFence.Destroy();
    context.copyFence.Destroy();

    context.copyQueue.Reset();
    context.computeQueue.Reset();
    context.graphicsQueue.Reset();

    if (context.allocator != nullptr)
    {
        context.allocator->Release();
        context.allocator = nullptr;
    }

    context.device.Reset();
    context.adapter.Reset();
    context.factory.Reset();
}

NAMESPACE_RENDERCORE_END
