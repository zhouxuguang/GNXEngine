//
//  DX12RenderDevice.cpp
//  rendercore
//

#include "DX12RenderDevice.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12Sampler.h"
#include "DX12ShaderFunction.h"
#include "DX12Pipeline.h"
#include "DX12CommandBuffer.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

#include <exception>

NAMESPACE_RENDERCORE_BEGIN

using namespace baselib;

DX12RenderDevice::DX12RenderDevice(const NativeWindow& nativeWindow)
{
    mInitialized = Initialize(nativeWindow);
}

DX12RenderDevice::~DX12RenderDevice()
{
    if (mContext)
    {
        if (mContext->graphicsFence.IsValid())
        {
            // 排空队列（含 Present 后 DXGI 追加的工作），否则销毁交换链时仍在执行
            mContext->graphicsFence.FlushAndWait(mContext->graphicsQueue.Get(), 5000);
        }
    }

    // 顺序很重要：帧资源 → 交换链 → 根签名 → 上下文
    ReleaseFrameResources();
    mSwapChain.reset();
    mRootSignature.reset();

    mGraphicsQueues.clear();
    mComputeQueues.clear();
    mTransferQueues.clear();

    if (mContext)
    {
        DX12DestroyContext(*mContext);
        mContext.reset();
    }
}

// Keep fatal diagnostics visible in demo and CLI hosts.
namespace
{
LONG WINAPI DX12UnhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
    LOG_ERROR("[DX12] Unhandled exception code=0x%08X address=%p",
              info && info->ExceptionRecord ? (unsigned)info->ExceptionRecord->ExceptionCode : 0u,
              info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr);
    return EXCEPTION_CONTINUE_SEARCH;
}

void DX12TerminateHandler()
{
    LOG_ERROR("[DX12] std::terminate called");
    abort();
}

void DX12InstallCrashDiagnostics()
{
    static bool installed = false;
    if (installed)
    {
        return;
    }
    installed = true;
    SetUnhandledExceptionFilter(&DX12UnhandledExceptionFilter);
    std::set_terminate(&DX12TerminateHandler);
}
} // namespace

#define DX12_INIT_TRACE(step) LOG_DEBUG("[DX12] init: %s", step)

bool DX12RenderDevice::Initialize(const NativeWindow& nativeWindow)
{
    DX12InstallCrashDiagnostics();
    DX12_INIT_TRACE("begin");
    mContext = std::make_shared<DX12Context>();

    DX12EnableDebugLayer(*mContext);
    DX12_INIT_TRACE("debug layer done");

    if (!DX12CreateFactory(*mContext))
    {
        DX12_INIT_TRACE("FAILED: create factory");
        return false;
    }
    DX12_INIT_TRACE("factory ok");

    if (!DX12SelectAdapter(*mContext))
    {
        DX12_INIT_TRACE("FAILED: select adapter");
        return false;
    }
    DX12_INIT_TRACE("adapter ok");

    if (!DX12CreateDevice(*mContext))
    {
        DX12_INIT_TRACE("FAILED: create device");
        return false;
    }
    DX12_INIT_TRACE("device ok");

    if (!DX12CreateAllocator(*mContext))
    {
        DX12_INIT_TRACE("FAILED: create allocator (D3D12MA)");
        return false;
    }
    DX12_INIT_TRACE("allocator ok");

    if (!DX12CreateCommandQueues(*mContext))
    {
        DX12_INIT_TRACE("FAILED: create command queues");
        return false;
    }
    DX12_INIT_TRACE("command queues ok");

    if (!DX12CreateSharedDescriptorPools(*mContext))
    {
        DX12_INIT_TRACE("FAILED: create descriptor pools");
        return false;
    }
    DX12_INIT_TRACE("descriptor pools ok");

    InitializeFeatures();
    DX12_INIT_TRACE("features ok");

    mRootSignature = std::make_unique<DX12RootSignature>();
    if (!mRootSignature->Init(mContext->device.Get()))
    {
        DX12_INIT_TRACE("FAILED: root signature");
        return false;
    }
    DX12_INIT_TRACE("root signature ok");

    // ---- 命令队列（RHI 抽象层）----
    mGraphicsQueues.push_back(std::make_shared<DX12CommandQueue>(
        this, mContext, QueueType::Graphics, mContext->graphicsQueue.Get(), 0, 0));
    if (mContext->computeQueue)
    {
        mComputeQueues.push_back(std::make_shared<DX12CommandQueue>(
            this, mContext, QueueType::Compute, mContext->computeQueue.Get(), 0, 1));
    }
    if (mContext->copyQueue)
    {
        mTransferQueues.push_back(std::make_shared<DX12CommandQueue>(
            this, mContext, QueueType::Transfer, mContext->copyQueue.Get(), 0, 2));
    }
    DX12_INIT_TRACE("rhi queues ok");

    CreateFrameResources();
    DX12_INIT_TRACE("frame resources ok");

    // ---- 交换链 ----
    HWND hwnd = static_cast<HWND>(nativeWindow.viewHandle);
    if (hwnd == nullptr)
    {
        DX12_INIT_TRACE("FAILED: NativeWindow.viewHandle is null");
        return false;
    }

    mSwapChain = std::make_shared<DX12SwapChain>();
    if (!mSwapChain->Init(mContext, hwnd, 1280, 720, mVSync))
    {
        DX12_INIT_TRACE("FAILED: swap chain init");
        mSwapChain.reset();
        return false;
    }
    DX12_INIT_TRACE("swap chain ok");

    mWidth = mSwapChain->GetWidth();
    mHeight = mSwapChain->GetHeight();

    DX12_INIT_TRACE("device fully initialized");

    LOG_INFO("[DX12] Render device initialized (%ux%u, vsync=%d, meshShader=%d)",
             mWidth, mHeight, (int)mVSync, (int)mFeatures.shader.meshShader);

    return true;
}

void DX12RenderDevice::CreateFrameResources()
{
    ReleaseFrameResources();

    mFrameCommandAllocators.resize(GNX_DX12_FRAME_COUNT);
    mFrameCommandLists.resize(GNX_DX12_FRAME_COUNT);
    mFrameFences.resize(GNX_DX12_FRAME_COUNT);
    mFrameDeferredReleases.resize(GNX_DX12_FRAME_COUNT);
    mFrameCommandBuffers.resize(GNX_DX12_FRAME_COUNT);

    for (uint32_t i = 0; i < GNX_DX12_FRAME_COUNT; ++i)
    {
        HRESULT hr = mContext->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                             IID_PPV_ARGS(&mFrameCommandAllocators[i]));
        if (FAILED(hr))
        {
            LOG_ERROR("[DX12] CreateCommandAllocator(%u) failed: %s", i, DX12HResultToString(hr));
            continue;
        }

        hr = mContext->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 mFrameCommandAllocators[i].Get(), nullptr,
                                                 IID_PPV_ARGS(&mFrameCommandLists[i]));
        if (FAILED(hr))
        {
            LOG_ERROR("[DX12] CreateCommandList(%u) failed: %s", i, DX12HResultToString(hr));
            continue;
        }
        // 创建后处于打开状态，立刻关闭以便后续 Reset
        mFrameCommandLists[i]->Close();

        mFrameFences[i].Init(mContext->device.Get(), 0, "DX12 Frame Fence");
    }
}

void DX12RenderDevice::ReleaseFrameResources()
{
    // 等待所有在飞帧完成，避免释放仍被 GPU 使用的资源
    for (auto& fence : mFrameFences)
    {
        if (fence.IsValid())
        {
            fence.WaitForIdle(5000);
            fence.Destroy();
        }
    }

    mFrameCommandBuffers.clear();
    mFrameDeferredReleases.clear();
    mFrameCommandLists.clear();
    mFrameCommandAllocators.clear();
    mFrameFences.clear();
}

void DX12RenderDevice::InitializeFeatures()
{
    const DX12Context& ctx = *mContext;
    auto& f = mFeatures;

    f.deviceInfo.deviceName = ctx.deviceName;
    f.deviceInfo.vendorName = ctx.vendorName;
    f.deviceInfo.deviceType = ctx.isDiscreteGPU
                                  ? RenderDeviceFeatures::DeviceInfo::DeviceType::Discrete
                                  : RenderDeviceFeatures::DeviceInfo::DeviceType::Integrated;
    f.deviceInfo.apiVersion = ctx.maxSupportedFeatureLevel;

    auto& L = f.limits;
    L.maxTextureSize2D    = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    L.maxTextureSize3D    = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    L.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    L.maxCubeMapSize      = D3D12_REQ_TEXTURECUBE_DIMENSION;
    L.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    L.maxVertexBufferBindings  = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    L.maxVertexInputAttributes = D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT;
    L.minUniformBufferOffsetAlignment = GNX_DX12_CONSTANT_BUFFER_ALIGNMENT;
    L.minStorageBufferOffsetAlignment = 16;   // D3D12 结构化缓冲的偏移要求
    L.maxUniformBufferRange = 65536;          // D3D12 CBV 上限
    L.maxStorageBufferRange = 0xFFFFFFFFu;
    // ResourceBindingTier 决定可用描述符数量（Tier3 = 全功能）
    L.maxSampleCountMaskBits = 0x01;          // v1 不使用 MSAA

    f.shader.meshShader = ctx.isMeshShaderSupported;
    f.shader.taskShader = ctx.isMeshShaderSupported;
    f.shader.waveIntrinsics = (ctx.options1.WaveOps != FALSE);
    f.shader.rayTracing = ctx.isRayTracingSupported;
    // float16 需要 SM 6.2；这里用已查询到的 SM 6.6 支持情况保守判断
    f.shader.float16 = ctx.isShaderModel6_6Supported;
    f.shader.int8 = false;
    f.shader.int64Atomics = (ctx.options1.Int64ShaderOps != FALSE);
    f.shader.float64 = false;

    f.resource.textureCompressionBC = true;    // 桌面 D3D12 必支持 BC
    f.resource.textureCompressionASTC = false;
    f.resource.textureCompressionETC2 = false;
    f.resource.textureCompressionPVRTC = false;
    f.resource.bindlessResources = ctx.isShaderModel6_6Supported;
    f.resource.sparseTextures = (ctx.options.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_1);
    f.resource.formatBGRA8 = true;

    // VRS 的 tier 在 D3D12_FEATURE_DATA_D3D12_OPTIONS6 中，v1 不使用 VRS
    f.advanced.variableRateShading = false;
    f.advanced.asyncCompute = (ctx.computeQueue != nullptr &&
                               ctx.computeQueue.Get() != ctx.graphicsQueue.Get());
    f.advanced.multiview = false;
}

// ============================================================================
// 尺寸与窗口生命周期
// ============================================================================

void DX12RenderDevice::Resize(uint32_t width, uint32_t height)
{
    if (!mSwapChain || width == 0 || height == 0)
    {
        return;
    }

    if (mContext->graphicsFence.IsValid())
    {
        // 必须先排空队列，否则 ResizeBuffers 会在 back buffer 仍被使用时释放内部对象
        mContext->graphicsFence.FlushAndWait(mContext->graphicsQueue.Get(), 5000);
    }

    if (!mSwapChain->Resize(width, height))
    {
        LOG_ERROR("[DX12] Swap chain resize failed (%ux%u)", width, height);
        return;
    }
    mWidth = width;
    mHeight = height;
    mCurrentFrameIndex = 0;
    mBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void DX12RenderDevice::OnWindowRestored(const NativeWindow& nativeWindow)
{
    if (!mSwapChain || nativeWindow.viewHandle == nullptr)
    {
        return;
    }

    if (mContext->graphicsFence.IsValid())
    {
        // 重建交换链前彻底排空队列
        mContext->graphicsFence.FlushAndWait(mContext->graphicsQueue.Get(), 5000);
    }

    mSwapChain->Recreate(mWidth ? mWidth : 1280, mHeight ? mHeight : 720, mVSync);
    mCurrentFrameIndex = 0;
}

void DX12RenderDevice::OnWindowMinimized()
{
    if (mContext && mContext->graphicsFence.IsValid())
    {
        mContext->graphicsFence.WaitForIdle(5000);
    }
}

// ============================================================================
// 帧管理
// ============================================================================

bool DX12RenderDevice::WaitForFrameSlot(uint32_t frameIndex, uint32_t timeoutMs)
{
    if (frameIndex >= mFrameFences.size())
    {
        return false;
    }
    if (!mFrameFences[frameIndex].IsValid())
    {
        return true;
    }
    return mFrameFences[frameIndex].WaitForIdle(timeoutMs);
}

void DX12RenderDevice::SignalFrameFence(uint32_t frameIndex)
{
    if (frameIndex >= mFrameFences.size() || !mFrameFences[frameIndex].IsValid())
    {
        return;
    }
    if (!mContext || !mContext->graphicsQueue)
    {
        return;
    }
    // 在图形队列上 signal：该槽位的命令列表执行完毕即代表本帧 GPU 工作结束
    mFrameFences[frameIndex].Signal(mContext->graphicsQueue.Get());
}

bool DX12RenderDevice::IsFrameSlotInFlight(uint32_t frameIndex) const
{
    if (frameIndex >= mFrameFences.size() || !mFrameFences[frameIndex].IsValid())
    {
        return false;
    }
    DX12Fence& fence = const_cast<DX12Fence&>(mFrameFences[frameIndex]);
    return fence.GetCompletedValue() < fence.GetLastSignaledValue();
}

void DX12RenderDevice::EnqueueFrameDeferredRelease(uint32_t frameIndex, std::function<void()>&& release)
{
    if (frameIndex >= mFrameDeferredReleases.size())
    {
        release();
        return;
    }
    mFrameDeferredReleases[frameIndex].push_back(std::move(release));
}

void DX12RenderDevice::AdvanceFrameIndex()
{
    if (mFrameCommandBuffers.empty())
    {
        return;
    }

    const uint32_t previous = mCurrentFrameIndex;
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % GNX_DX12_FRAME_COUNT;

    // 回收上一帧槽位的延迟释放队列（该槽位的 GPU 工作此时已完成）
    if (previous < mFrameDeferredReleases.size())
    {
        for (auto& release : mFrameDeferredReleases[previous])
        {
            if (release)
            {
                release();
            }
        }
        mFrameDeferredReleases[previous].clear();
    }
}

CommandBufferPtr DX12RenderDevice::CreateCommandBuffer()
{
    if (!mSwapChain || !mSwapChain->IsValid() || mFrameCommandLists.empty())
    {
        LOG_ERROR("[DX12] Cannot create command buffer: swap chain or command lists unavailable");
        return nullptr;
    }

    // 等待当前帧槽位的上一轮 GPU 工作完成（有限超时，避免卡死整个渲染循环）
    if (!WaitForFrameSlot(mCurrentFrameIndex, 5000))
    {
        LOG_WARN("[DX12] Frame slot %u wait timed out, skipping frame", mCurrentFrameIndex);
        return nullptr;
    }

    if (!mSwapChain->BeginFrame())
    {
        LOG_WARN("[DX12] Swap chain failed to begin frame");
        return nullptr;
    }
    mBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    // 复用该槽位的命令缓冲区（缓存描述符堆，避免每帧重建堆的开销）
    DX12CommandBufferPtr& cached = mFrameCommandBuffers[mCurrentFrameIndex];
    if (!cached)
    {
        auto info = std::make_shared<DX12CommandBufferInfo>();
        info->context = mContext;
        info->swapChain = mSwapChain;
        info->renderDevice = this;
        info->frameIndex = mCurrentFrameIndex;
        info->backBufferIndex = mBackBufferIndex;

        cached = std::make_shared<DX12CommandBuffer>(
            mFrameCommandLists[mCurrentFrameIndex].Get(),
            mFrameCommandAllocators[mCurrentFrameIndex].Get(), info);
    }

    cached->GetFrameInfo()->backBufferIndex = mBackBufferIndex;

    if (!cached->BeginRecording())
    {
        LOG_ERROR("[DX12] Command buffer failed to begin recording");
        return nullptr;
    }

    return cached;
}

// ============================================================================
// 资源创建
// ============================================================================

RCBufferPtr DX12RenderDevice::CreateBuffer(const RCBufferDesc& desc) const
{
    return std::make_shared<DX12RCBuffer>(mContext, desc);
}

RCBufferPtr DX12RenderDevice::CreateBuffer(const RCBufferDesc& desc, const void* data) const
{
    return std::make_shared<DX12RCBuffer>(mContext, desc, data);
}

TextureSamplerPtr DX12RenderDevice::CreateSamplerWithDescriptor(const SamplerDesc& des) const
{
    return std::make_shared<DX12TextureSampler>(des);
}

UniformBufferPtr DX12RenderDevice::CreateUniformBufferWithSize(uint32_t bufSize) const
{
    return std::make_shared<DX12UniformBuffer>(mContext, bufSize);
}

GraphicsShaderPtr DX12RenderDevice::CreateGraphicsShader(const ShaderCode& vertexShader,
                                                         const ShaderCode& fragmentShader) const
{
    return std::make_shared<DX12GraphicsShader>(mContext, vertexShader, fragmentShader);
}

GraphicsShaderPtr DX12RenderDevice::CreateGraphicsShader(const ShaderStageData& vertexShader,
                                                         const ShaderStageData& fragmentShader) const
{
    return std::make_shared<DX12GraphicsShader>(mContext, vertexShader, fragmentShader);
}

GraphicsShaderPtr DX12RenderDevice::CreateMeshGraphicsShader(const ShaderCode& taskShader,
                                                             const ShaderCode& meshShader,
                                                             const ShaderCode& fragmentShader) const
{
    return std::make_shared<DX12GraphicsShader>(mContext, taskShader, meshShader, fragmentShader);
}

GraphicsShaderPtr DX12RenderDevice::CreateMeshGraphicsShader(const ShaderStageData& taskShader,
                                                             const ShaderStageData& meshShader,
                                                             const ShaderStageData& fragmentShader) const
{
    return std::make_shared<DX12GraphicsShader>(mContext, taskShader, meshShader, fragmentShader);
}

GraphicsPipelinePtr DX12RenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& des) const
{
    auto pipeline = std::make_shared<DX12GraphicsPipeline>(mContext, mRootSignature.get(), des);
    return pipeline;
}

ComputePipelinePtr DX12RenderDevice::CreateComputePipeline(const ShaderCode& shaderString) const
{
    return std::make_shared<DX12ComputePipeline>(mContext, mRootSignature.get(), shaderString);
}

ComputePipelinePtr DX12RenderDevice::CreateComputePipeline(const ShaderStageData& shader) const
{
    return std::make_shared<DX12ComputePipeline>(mContext, mRootSignature.get(), shader);
}

RCTexture2DPtr DX12RenderDevice::CreateTexture2D(TextureFormat format, TextureUsage usage,
                                                 uint32_t width, uint32_t height,
                                                 uint32_t levels) const
{
    if (width == 0 || height == 0 || levels == 0)
    {
        return nullptr;
    }

    const DXGI_FORMAT dxgiFormat = DX12Util::ConvertTextureFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return nullptr;
    }

    D3D12_RESOURCE_DESC desc = DX12TextureResourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, 1, levels, dxgiFormat,
        DX12Util::ConvertTextureUsage(mContext->device.Get(), usage, dxgiFormat));
    // A sampled-only texture must start in COMMON for a COPY command queue.
    // The format capability probe above also enables RT/UAV flags, which
    // otherwise makes the texture start in RENDER_TARGET state unnecessarily.
    if (usage == TextureUsage::TextureUsageShaderRead)
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    auto texture = std::make_shared<DX12RCTexture2D>(mContext, desc, format);
    if (!texture->IsValid())
    {
        return nullptr;
    }
    return texture;
}

RCTexture3DPtr DX12RenderDevice::CreateTexture3D(TextureFormat format, TextureUsage usage,
                                                 uint32_t width, uint32_t height, uint32_t depth,
                                                 uint32_t levels) const
{
    if (width == 0 || height == 0 || depth == 0 || levels == 0)
    {
        return nullptr;
    }

    const DXGI_FORMAT dxgiFormat = DX12Util::ConvertTextureFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return nullptr;
    }

    const D3D12_RESOURCE_DESC desc = DX12TextureResourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE3D, width, height, depth, levels, dxgiFormat,
        DX12Util::ConvertTextureUsage(mContext->device.Get(), usage, dxgiFormat));

    auto texture = std::make_shared<DX12RCTexture3D>(mContext, desc, format);
    if (!texture->IsValid())
    {
        return nullptr;
    }
    return texture;
}

RCTextureCubePtr DX12RenderDevice::CreateTextureCube(TextureFormat format, TextureUsage usage,
                                                     uint32_t width, uint32_t height,
                                                     uint32_t levels) const
{
    if (width == 0 || height == 0 || levels == 0 || width != height)
    {
        LOG_ERROR("[DX12] CreateTextureCube: 要求 width == height 且均非 0（%ux%u）", width, height);
        return nullptr;
    }

    const DXGI_FORMAT dxgiFormat = DX12Util::ConvertTextureFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return nullptr;
    }

    // Cube 在 D3D12 里是 6 层的 2D 数组
    const D3D12_RESOURCE_DESC desc = DX12TextureResourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, 6, levels, dxgiFormat,
        DX12Util::ConvertTextureUsage(mContext->device.Get(), usage, dxgiFormat));

    auto texture = std::make_shared<DX12RCTextureCube>(mContext, desc, format);
    if (!texture->IsValid())
    {
        return nullptr;
    }
    return texture;
}

RCTexture2DArrayPtr DX12RenderDevice::CreateTexture2DArray(TextureFormat format, TextureUsage usage,
                                                          uint32_t width, uint32_t height,
                                                          uint32_t levels, uint32_t arraySize) const
{
    if (width == 0 || height == 0 || levels == 0 || arraySize == 0)
    {
        return nullptr;
    }

    const DXGI_FORMAT dxgiFormat = DX12Util::ConvertTextureFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return nullptr;
    }

    const D3D12_RESOURCE_DESC desc = DX12TextureResourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, arraySize, levels, dxgiFormat,
        DX12Util::ConvertTextureUsage(mContext->device.Get(), usage, dxgiFormat));

    auto texture = std::make_shared<DX12RCTexture2DArray>(mContext, desc, format);
    if (!texture->IsValid())
    {
        return nullptr;
    }
    return texture;
}

// ============================================================================
// 队列查询 / VSync / 杂项
// ============================================================================

CommandQueuePtr DX12RenderDevice::GetCommandQueue(QueueType type, uint32_t index) const
{
    switch (type)
    {
        case QueueType::Graphics:
            if (index < mGraphicsQueues.size()) return mGraphicsQueues[index];
            break;
        case QueueType::Compute:
            if (index < mComputeQueues.size()) return mComputeQueues[index];
            break;
        case QueueType::Transfer:
            if (index < mTransferQueues.size()) return mTransferQueues[index];
            break;
        default:
            break;
    }
    return nullptr;
}

uint32_t DX12RenderDevice::GetCommandQueueCount(QueueType type) const
{
    switch (type)
    {
        case QueueType::Graphics: return (uint32_t)mGraphicsQueues.size();
        case QueueType::Compute:  return (uint32_t)mComputeQueues.size();
        case QueueType::Transfer: return (uint32_t)mTransferQueues.size();
        default:                  return 0;
    }
}

void DX12RenderDevice::SetVSync(bool enable)
{
    if (mVSync == enable)
    {
        return;
    }
    mVSync = enable;

    // Present() 读的是交换链自己的 mVSync，必须同步过去
    if (mSwapChain)
    {
        mSwapChain->SetVSync(enable);
    }
}

bool DX12RenderDevice::IsVSync() const
{
    return mSwapChain ? mSwapChain->IsVSync() : mVSync;
}

void DX12RenderDevice::FlushPipelineCache()
{
    if (mContext)
    {
        mContext->LogMemoryStatistics();
    }
}

NAMESPACE_RENDERCORE_END
