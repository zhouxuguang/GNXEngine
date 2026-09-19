//
//  DX12SwapChain.h
//  rendercore
//
//  DXGI 交换链。
//
//  与 Vulkan 后端的差异：
//   - DXGI 自己管理 back buffer 的获取与轮转（无 vkAcquireNextImageKHR），
//     因此这里只需暴露 CurrentBackBufferIndex 并处理 Present 的返回码。
//   - 深度缓冲按 back buffer 逐个分配（而非全局一个）。因为引擎是
//     多帧在飞（GNX_DX12_FRAME_COUNT），两个在飞帧若共用同一个深度缓冲，
//     后一帧的 clear 会与前一帧的深度写入产生竞争。
//   - back buffer 通过 DX12WrapExternalTexture2D 包装成普通 RHI 纹理，
//     使 CreateDefaultRenderEncoder 可以像处理离屏 RT 一样处理上屏渲染。
//

#ifndef GNX_ENGINE_DX12_SWAPCHAIN_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_SWAPCHAIN_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "DX12Texture.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12SwapChain
{
public:
    DX12SwapChain() = default;
    ~DX12SwapChain();

    DX12SwapChain(const DX12SwapChain&) = delete;
    DX12SwapChain& operator=(const DX12SwapChain&) = delete;

    /// 首次创建（会创建 swap chain、back buffer 包装、RTV、逐帧深度缓冲）
    ///
    /// 持有 context 的 shared_ptr：包装 back buffer 的 RHI 纹理需要长期引用 context，
    /// 这样才能保证纹理生命周期内的上下文有效性（并避免使用别名 shared_ptr 的 hack）。
    bool Init(const DX12ContextPtr& context, HWND hwnd, uint32_t width, uint32_t height, bool vsync);

    /// 释放全部资源（不释放 context）
    void Release();

    /// 重建（Resize / 切换 VSync / 交换链失效）
    bool Recreate(uint32_t width, uint32_t height, bool vsync);

    /// ResizeBuffers 路径（尺寸变化时优先走这个，避免重建交换链）
    bool Resize(uint32_t width, uint32_t height);

    bool IsValid() const { return mSwapChain != nullptr; }

    /**
     * @brief 开始新的一帧
     *
     * 处理 Present 返回的 OCCLUDED / OUT_OF_DATE：必要时自动重建交换链。
     * @return 成功返回 true；失败（含设备移除）返回 false，调用方应跳过本帧
     */
    bool BeginFrame();

    /**
     * @brief 上屏
     * @return 成功返回 true；设备移除类错误返回 false
     */
    bool Present();

    uint32_t GetCurrentBackBufferIndex() const { return mCurrentBackBufferIndex; }
    uint32_t GetBackBufferCount() const { return (uint32_t)mBackBuffers.size(); }

    ID3D12Resource* GetBackBuffer(uint32_t index) const;
    const std::shared_ptr<DX12RCTexture2D>& GetBackBufferTexture(uint32_t index) const;

    const std::shared_ptr<DX12RCTexture2D>& GetCurrentBackBufferTexture() const;
    const std::shared_ptr<DX12RCTexture2D>& GetCurrentDepthTexture() const;
    const std::shared_ptr<DX12RCTexture2D>& GetDepthTexture(uint32_t index) const;

    uint32_t GetWidth() const { return mWidth; }
    uint32_t GetHeight() const { return mHeight; }
    DXGI_FORMAT GetFormat() const { return mFormat; }
    bool IsVSync() const { return mVSync; }

    // Present 时决定 syncInterval/presentFlags，无需重建交换链
    void SetVSync(bool vsync) { mVSync = vsync; }

    /// 是否发生了“交换链已重建”，供上层重置帧索引
    bool ConsumeResizedFlag();

private:
    bool CreateSwapChainInternal();
    bool CreateBackBufferTextures();
    bool CreateDepthTextures();
    void ReleaseBackBufferResources();

    DX12ContextPtr mContextPtr;      // 持有引用，保证生命周期覆盖到 back buffer 纹理
    DX12Context* mContext = nullptr; // 便捷裸指针
    HWND mHwnd = nullptr;

    ComPtr<IDXGISwapChain4> mSwapChain;
    std::vector<ComPtr<ID3D12Resource>> mBackBuffers;
    std::vector<std::shared_ptr<DX12RCTexture2D>> mBackBufferTextures;   // 包装后的 RHI 纹理
    std::vector<std::shared_ptr<DX12RCTexture2D>> mDepthTextures;        // 逐 back buffer 的深度

    DXGI_FORMAT mFormat = GNX_DX12_BACKBUFFER_FORMAT;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mCurrentBackBufferIndex = 0;
    bool mVSync = true;
    bool mWasResized = false;
};

using DX12SwapChainPtr = std::shared_ptr<DX12SwapChain>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_SWAPCHAIN_INCLUDE_JHGSD */
