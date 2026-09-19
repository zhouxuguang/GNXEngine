//
//  DX12SwapChain.cpp
//  rendercore
//

#include "DX12SwapChain.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

NAMESPACE_RENDERCORE_BEGIN

DX12SwapChain::~DX12SwapChain()
{
    Release();
}

bool DX12SwapChain::Init(const DX12ContextPtr& context, HWND hwnd, uint32_t width, uint32_t height,
                         bool vsync)
{
    if (context == nullptr || !context->IsValid() || hwnd == nullptr || width == 0 || height == 0)
    {
        LOG_ERROR("[DX12] DX12SwapChain::Init invalid args (hwnd=%p, %ux%u)", (void*)hwnd, width, height);
        return false;
    }

    mContextPtr = context;
    mContext = context.get();
    mHwnd = hwnd;
    mWidth = width;
    mHeight = height;
    mVSync = vsync;

    if (!CreateSwapChainInternal())
    {
        return false;
    }

    if (!CreateBackBufferTextures())
    {
        return false;
    }

    return CreateDepthTextures();
}

bool DX12SwapChain::CreateSwapChainInternal()
{
    // 使用 FLIP_DISCARD 模型（D3D12 上的推荐模型；BITBLT 模型已过时且限制更多）
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width              = mWidth;
    desc.Height             = mHeight;
    desc.Format             = mFormat;
    desc.Stereo             = FALSE;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount        = GNX_DX12_FRAME_COUNT;
    desc.Scaling            = DXGI_SCALING_STRETCH;
    desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;   // 允许无 VSync 时的撕裂模式

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = mContext->factory->CreateSwapChainForHwnd(
        mContext->graphicsQueue.Get(), mHwnd, &desc, nullptr, nullptr, &swapChain1);
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] CreateSwapChainForHwnd failed: %s (%ux%u)",
                  DX12HResultToString(hr), mWidth, mHeight);
        return false;
    }

    // 禁用 Alt+Enter 自动全屏（由应用自己控制）
    mContext->factory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER);

    hr = swapChain1.As(&mSwapChain);
    if (FAILED(hr) || !mSwapChain)
    {
        LOG_ERROR("[DX12] Failed to query IDXGISwapChain4: %s", DX12HResultToString(hr));
        return false;
    }

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12SwapChain::CreateBackBufferTextures()
{
    ReleaseBackBufferResources();

    if (!mSwapChain)
    {
        return false;
    }

    const uint32_t count = GNX_DX12_FRAME_COUNT;
    mBackBuffers.resize(count);
    mBackBufferTextures.resize(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        HRESULT hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i]));
        if (FAILED(hr) || !mBackBuffers[i])
        {
            LOG_ERROR("[DX12] IDXGISwapChain::GetBuffer(%u) failed: %s", i, DX12HResultToString(hr));
            return false;
        }

        wchar_t name[64] = {};
        swprintf_s(name, L"DX12 BackBuffer %u", i);
        mBackBuffers[i]->SetName(name);

        // 包装成 RHI 纹理，初始状态必须是 PRESENT（FLIP 模型下 back buffer 由 DXGI 持有）
        mBackBufferTextures[i] = DX12WrapExternalTexture2D(
            mContextPtr, mBackBuffers[i].Get(), mFormat, mWidth, mHeight,
            D3D12_RESOURCE_STATE_PRESENT);

        if (!mBackBufferTextures[i])
        {
            LOG_ERROR("[DX12] Failed to wrap back buffer %u as an RHI texture", i);
            return false;
        }
    }

    return true;
}

bool DX12SwapChain::CreateDepthTextures()
{
    mDepthTextures.clear();
    mDepthTextures.resize(GNX_DX12_FRAME_COUNT);

    for (uint32_t i = 0; i < GNX_DX12_FRAME_COUNT; ++i)
    {
        const D3D12_RESOURCE_DESC desc = DX12TextureResourceDesc(
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            mWidth, mHeight, 1, 1,
            GNX_DX12_DEPTH_FORMAT,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        auto depthTexture = std::make_shared<DX12RCTexture2D>(
            mContextPtr, desc, kTexFormatDepth32Float);

        if (!depthTexture->IsValid())
        {
            LOG_ERROR("[DX12] Failed to create depth texture %u (%ux%u)", i, mWidth, mHeight);
            return false;
        }

        wchar_t name[64] = {};
        swprintf_s(name, L"DX12 SwapChain Depth %u", i);
        depthTexture->SetName("DX12SwapChainDepth");
        if (depthTexture->GetResource())
        {
            depthTexture->GetResource()->SetName(name);
        }

        mDepthTextures[i] = depthTexture;
    }

    return true;
}

void DX12SwapChain::ReleaseBackBufferResources()
{
    // 释放前必须解除包装纹理对资源的引用，否则 ResizeBuffers 会因资源被占用而失败
    mBackBufferTextures.clear();
    mBackBuffers.clear();
}

void DX12SwapChain::Release()
{
    mDepthTextures.clear();
    ReleaseBackBufferResources();

    if (mSwapChain)
    {
        // 退出全屏等状态清理
        mSwapChain->SetFullscreenState(FALSE, nullptr);
        mSwapChain.Reset();
    }

    mContextPtr.reset();
    mContext = nullptr;
    mHwnd = nullptr;
}

bool DX12SwapChain::Resize(uint32_t width, uint32_t height)
{
    if (mSwapChain == nullptr || width == 0 || height == 0)
    {
        return false;
    }

    if (width == mWidth && height == mHeight)
    {
        return true;
    }

    mWidth = width;
    mHeight = height;

    // ResizeBuffers 要求释放所有对 back buffer 的引用
    mDepthTextures.clear();
    ReleaseBackBufferResources();

    const HRESULT hr = mSwapChain->ResizeBuffers(GNX_DX12_FRAME_COUNT, mWidth, mHeight,
                                                 mFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] ResizeBuffers failed: %s (%ux%u)", DX12HResultToString(hr), mWidth, mHeight);
        return false;
    }

    if (!CreateBackBufferTextures() || !CreateDepthTextures())
    {
        return false;
    }

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    mWasResized = true;
    return true;
}

bool DX12SwapChain::Recreate(uint32_t width, uint32_t height, bool vsync)
{
    if (mContextPtr == nullptr || mHwnd == nullptr)
    {
        return false;
    }

    // 先把 GPU 工作排空，避免销毁仍在使用的 back buffer
    if (mContextPtr->graphicsFence.IsValid())
    {
        mContextPtr->graphicsFence.FlushAndWait(mContextPtr->graphicsQueue.Get(), 5000);
    }

    // Release 会清空 mContextPtr / mHwnd，因此先保存
    const DX12ContextPtr context = mContextPtr;
    HWND hwnd = mHwnd;

    Release();

    return Init(context, hwnd, width, height, vsync);
}

bool DX12SwapChain::BeginFrame()
{
    if (!mSwapChain || !mContext)
    {
        return false;
    }

    // 检测设备移除：这是 D3D12 上必须显式处理的错误路径
    const HRESULT reason = mContext->device->GetDeviceRemovedReason();
    if (FAILED(reason))
    {
        LOG_ERROR("[DX12] Device removed (reason=0x%08X): %s",
                  (unsigned)reason, DX12HResultToString(reason));
        return false;
    }

    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12SwapChain::Present()
{
    if (!mSwapChain)
    {
        return false;
    }

    const UINT syncInterval = mVSync ? 1u : 0u;
    // 关闭 VSync 时使用 tearing 标志（需要 ALLOW_TEARING + FLIP 模型）
    const UINT presentFlags = mVSync ? 0u : DXGI_PRESENT_ALLOW_TEARING;

    const HRESULT hr = mSwapChain->Present(syncInterval, presentFlags);

    if (hr == DXGI_STATUS_OCCLUDED)
    {
        // 窗口被遮挡（最小化）：不视为错误，跳过本帧
        return true;
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        LOG_ERROR("[DX12] Present: device removed/reset: %s", DX12HResultToString(hr));
        return false;
    }

    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] Present failed: %s", DX12HResultToString(hr));
        return false;
    }

    return true;
}

ID3D12Resource* DX12SwapChain::GetBackBuffer(uint32_t index) const
{
    if (index >= mBackBuffers.size())
    {
        return nullptr;
    }
    return mBackBuffers[index].Get();
}

const std::shared_ptr<DX12RCTexture2D>& DX12SwapChain::GetBackBufferTexture(uint32_t index) const
{
    static const std::shared_ptr<DX12RCTexture2D> kNull;
    if (index >= mBackBufferTextures.size())
    {
        return kNull;
    }
    return mBackBufferTextures[index];
}

const std::shared_ptr<DX12RCTexture2D>& DX12SwapChain::GetCurrentBackBufferTexture() const
{
    return GetBackBufferTexture(mCurrentBackBufferIndex);
}

const std::shared_ptr<DX12RCTexture2D>& DX12SwapChain::GetDepthTexture(uint32_t index) const
{
    static const std::shared_ptr<DX12RCTexture2D> kNull;
    if (index >= mDepthTextures.size())
    {
        return kNull;
    }
    return mDepthTextures[index];
}

const std::shared_ptr<DX12RCTexture2D>& DX12SwapChain::GetCurrentDepthTexture() const
{
    return GetDepthTexture(mCurrentBackBufferIndex);
}

bool DX12SwapChain::ConsumeResizedFlag()
{
    const bool resized = mWasResized;
    mWasResized = false;
    return resized;
}

NAMESPACE_RENDERCORE_END
