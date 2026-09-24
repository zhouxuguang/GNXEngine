//
//  DX12Texture.h
//  rendercore
//
//  D3D12 纹理实现。
//
//  设计要点：
//  1) 深度格式一律以 typeless 格式创建资源，DSV 用带类型格式，SRV 用 R* 格式。
//     这是 D3D12 的硬性要求（否则深度纹理无法被采样，SSAO / HiZ / SSR 等
//     依赖深度采样的 pass 会直接失败）。
//  2) SRV/UAV 不缓存：调用方直接把目标描述符句柄传进来（写入命令缓冲区的
//     单帧环形堆），避免跨堆拷贝与缓存失效。
//  3) RTV/DSV 惰性分配并缓存：它们通过 OMSetRenderTargets 直接消费，且在
//     纹理生命周期内不变。
//

#ifndef GNX_ENGINE_DX12_TEXTURE_INCLUDE_JHGDSF
#define GNX_ENGINE_DX12_TEXTURE_INCLUDE_JHGDSF

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "RCTexture.h"

NAMESPACE_RENDERCORE_BEGIN

// SRV/UAV 写入时的哨兵值
constexpr uint32_t DX12_ALL_MIPS   = 0xFFFFFFFFu;
constexpr uint32_t DX12_ALL_SLICES = 0xFFFFFFFFu;

class DX12TextureBase : virtual public RCTexture
{
public:
    DX12TextureBase(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                    TextureFormat format);

    /**
     * @brief 包装一个外部拥有的 ID3D12Resource（交换链 back buffer）
     *
     * 这种情况下纹理不持有资源引用，也不负责释放；仅用于让编码器把
     * back buffer 当作普通 RHI 纹理来处理。
     */
    DX12TextureBase(const DX12ContextPtr& context, ID3D12Resource* externalResource,
                    DXGI_FORMAT viewFormat, uint32_t width, uint32_t height,
                    D3D12_RESOURCE_STATES currentState);

    virtual ~DX12TextureBase();

    // ---- RCTexture 接口 ----
    bool IsValid() const override;
    uint32_t GetWidth() const override { return mWidth; }
    uint32_t GetHeight() const override { return mHeight; }
    uint32_t GetDepth() const override { return mDepth; }
    uint32_t GetMipLevels() const override { return mMipLevels; }
    uint32_t GetLayerCount() const override { return mLayerCount; }
    void SetName(const char* name) override;

    /**
     * @brief 上传像素数据到纹理
     *
     * 内部使用 UPLOAD 堆暂存 + 一次性命令列表提交并等待完成。
     * 主要用于资产加载阶段（与 Vulkan 后端的 BeginSingleTimeCommand 语义一致）。
     */
    virtual void ReplaceRegion(const Rect2D& rect,
                               uint32_t level,
                               uint32_t slice,
                               const uint8_t* pixelBytes,
                               uint32_t bytesPerRow,
                               uint32_t bytesPerImage);

    // ---- D3D12 访问 ----
    ID3D12Resource* GetResource() const { return mResource.Get(); }
    DX12ContextPtr GetDX12Context() const { return mContext; }
    DXGI_FORMAT GetDXGIFormat() const { return mFormat; }
    DXGI_FORMAT GetTypelessFormat() const { return mTypelessFormat; }

    D3D12_RESOURCE_STATES GetCurrentState() const { return mCurrentState; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { mCurrentState = state; }

    D3D12MA::Allocation* GetAllocation() const { return mAllocation; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
    uint64_t GetSizeInBytes() const;

    bool IsDepthStencil() const { return mIsDepthStencil; }

    /**
     * @brief 把纹理的 SRV 写入指定描述符位置
     * @param mipLevel 起始 mip；DX12_ALL_MIPS 表示从 0 到末尾
     * @param firstSlice 起始数组层
     * @param sliceCount 层数；DX12_ALL_SLICES 表示到末尾
     */
    void WriteSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest,
                  uint32_t mipLevel = DX12_ALL_MIPS,
                  uint32_t firstSlice = 0,
                  uint32_t sliceCount = DX12_ALL_SLICES);

    /// 写入 UAV（仅 texture2d / texture2darray / texture3d；不支持多维数组切片）
    void WriteUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest,
                  uint32_t mipLevel = 0);

    /// 获取 RTV 句柄（惰性创建并缓存），仅颜色纹理有效
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t mipLevel = 0, uint32_t slice = 0);

    /// 获取 DSV 句柄（惰性创建并缓存），仅深度纹理有效
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

    /// 清除值（用于 RT/DS 优化清除值，避免调试层 MISMATCHINGCLEARVALUE 警告）
    D3D12_CLEAR_VALUE GetOptimizedClearValue(bool depth) const;

protected:
    /// 标记为立方体贴图（由 DX12RCTextureCube 构造时调用）
    void MarkAsCube() { mIsCube = true; }

    /// 记录初始的“自然”资源状态（ReplaceRegion 结束后会恢复到该状态）
    void SetNaturalState(D3D12_RESOURCE_STATES state)
    {
        mNaturalState = state;
        mCurrentState = state;
    }

    uint32_t GetSubresourceIndex(uint32_t mip, uint32_t slice) const
    {
        return mip + slice * mMipLevels;
    }

private:
    bool CreateResource(const D3D12_RESOURCE_DESC& desc);
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateRTV(uint32_t mipLevel, uint32_t slice);
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateDSV();

    /// 计算某个 mip 的尺寸
    void GetMipSize(uint32_t mip, uint32_t& w, uint32_t& h) const;

    DX12ContextPtr mContext;
    ComPtr<ID3D12Resource> mResource;
    D3D12MA::Allocation* mAllocation = nullptr;

    DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;        // 视图/采样使用的格式
    DXGI_FORMAT mTypelessFormat = DXGI_FORMAT_UNKNOWN;// 资源实际创建的格式（深度时为 typeless）

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mDepth = 0;
    uint32_t mMipLevels = 1;
    uint32_t mLayerCount = 1;

    D3D12_RESOURCE_DIMENSION mResourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    bool mIsCube = false;
    bool mIsDepthStencil = false;
    bool mIsExternal = false;   // 包装外部资源（交换链 back buffer），不持有所有权

    D3D12_RESOURCE_STATES mCurrentState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES mNaturalState = D3D12_RESOURCE_STATE_COMMON;

    // RTV/DSV 惰性缓存：key = mip * 1024 + slice（mip/slice 数量都很小）
    std::unordered_map<uint32_t, D3D12_CPU_DESCRIPTOR_HANDLE> mRTVCache;
    D3D12_CPU_DESCRIPTOR_HANDLE mDSVHandle = {};
    bool mDSVCreated = false;

    std::string mDebugName;
};

using DX12TextureBasePtr = std::shared_ptr<DX12TextureBase>;

// ---------------------------------------------------------------------------
// 具体纹理类型
//
// 注意：RCTexture 是虚基类且没有默认构造函数，必须由最派生的类显式构造；
// 同时 RCTexture2D 的 ReplaceRegion 是 4 参数版本，需要单独实现并转发。
// ---------------------------------------------------------------------------

class DX12RCTexture2D : public DX12TextureBase, public RCTexture2D,
                        public std::enable_shared_from_this<DX12RCTexture2D>
{
public:
    DX12RCTexture2D(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                    TextureFormat format)
        : RCTexture(TextureType_2D)
        , DX12TextureBase(context, desc, format)
    {
    }

    /// 包装外部资源（交换链 back buffer），见 DX12WrapExternalTexture2D
    DX12RCTexture2D(const DX12ContextPtr& context, ID3D12Resource* externalResource,
                    DXGI_FORMAT viewFormat, uint32_t width, uint32_t height,
                    D3D12_RESOURCE_STATES currentState)
        : RCTexture(TextureType_2D)
        , DX12TextureBase(context, externalResource, viewFormat, width, height, currentState)
    {
    }

    ~DX12RCTexture2D() override = default;

    void ReplaceRegion(const Rect2D& rect, uint32_t level,
                       const uint8_t* pixelBytes, uint32_t bytesPerRow) override
    {
        DX12TextureBase::ReplaceRegion(rect, level, 0, pixelBytes, bytesPerRow, 0);
    }

    TextureUploadPtr ReplaceRegionAsync(const Rect2D& rect, uint32_t level,
                       const uint8_t* pixelBytes, uint32_t bytesPerRow) override;
};

using DX12RCTexture2DPtr = std::shared_ptr<DX12RCTexture2D>;

class DX12RCTexture3D : public DX12TextureBase, public RCTexture3D
{
public:
    DX12RCTexture3D(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                    TextureFormat format)
        : RCTexture(TextureType_3D)
        , DX12TextureBase(context, desc, format)
    {
    }

    ~DX12RCTexture3D() override = default;

    // 必须显式重写：RCTexture3D 的纯虚函数与 DX12TextureBase 的同名函数
    // 来自两个不相关基类，不会自动互相 override（否则本类仍是抽象类）
    void ReplaceRegion(const Rect2D& rect, uint32_t level, uint32_t slice,
                       const uint8_t* pixelBytes, uint32_t bytesPerRow,
                       uint32_t bytesPerImage) override
    {
        DX12TextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
    }
};

using DX12RCTexture3DPtr = std::shared_ptr<DX12RCTexture3D>;

class DX12RCTextureCube : public DX12TextureBase, public RCTextureCube
{
public:
    DX12RCTextureCube(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                      TextureFormat format)
        : RCTexture(TextureType_CUBE)
        , DX12TextureBase(context, desc, format)
    {
        MarkAsCube();
    }

    ~DX12RCTextureCube() override = default;

    void ReplaceRegion(const Rect2D& rect, uint32_t level, uint32_t slice,
                       const uint8_t* pixelBytes, uint32_t bytesPerRow,
                       uint32_t bytesPerImage) override
    {
        DX12TextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
    }
};

using DX12RCTextureCubePtr = std::shared_ptr<DX12RCTextureCube>;

class DX12RCTexture2DArray : public DX12TextureBase, public RCTexture2DArray
{
public:
    DX12RCTexture2DArray(const DX12ContextPtr& context, const D3D12_RESOURCE_DESC& desc,
                         TextureFormat format)
        : RCTexture(TextureType_2D_ARRAY)
        , DX12TextureBase(context, desc, format)
    {
    }

    ~DX12RCTexture2DArray() override = default;

    void ReplaceRegion(const Rect2D& rect, uint32_t level, uint32_t slice,
                       const uint8_t* pixelBytes, uint32_t bytesPerRow,
                       uint32_t bytesPerImage) override
    {
        DX12TextureBase::ReplaceRegion(rect, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
    }
};

using DX12RCTexture2DArrayPtr = std::shared_ptr<DX12RCTexture2DArray>;

/// 用已有的 ID3D12Resource 包装成 RHI 纹理（用于交换链 back buffer，资源由 DXGI 拥有）
std::shared_ptr<DX12RCTexture2D> DX12WrapExternalTexture2D(const DX12ContextPtr& context,
                                                           ID3D12Resource* resource,
                                                           DXGI_FORMAT viewFormat,
                                                           uint32_t width, uint32_t height,
                                                           D3D12_RESOURCE_STATES currentState);

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_TEXTURE_INCLUDE_JHGDSF */
