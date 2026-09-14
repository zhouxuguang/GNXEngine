//
//  DX12RootSignature.h
//  rendercore
//
//  根签名。
//
//  ── 为什么只需要极少数几个根签名 ──
//  引擎的绑定模型是统一且扁平的：4 类资源（CBV/SRV/UAV/Sampler）×
//  按 HLSL 寄存器号索引。因此所有管线共享同一份根签名布局：
//
//      root param 0 : descriptor table → CBV     (register b0..b15)
//      root param 1 : descriptor table → SRV     (register t0..t15)
//      root param 2 : descriptor table → UAV     (register u0..u15)
//      root param 3 : descriptor table → Sampler (register s0..s15)
//
//  对 Mesh 管线额外提供一份“MESH + PIXEL 双可见”的变体：
//  D3D12 不允许对 amplification/mesh 阶段使用 SHADER_VISIBILITY_ALL，
//  但 Mesh 管线同时含 MS 与 PS，两侧都要看到资源，所以把 4 张表各注册两遍
//  （0..3 给 MESH，4..7 给 PIXEL），总计 8 个根参数，开销可忽略。
//

#ifndef GNX_ENGINE_DX12_ROOT_SIGNATURE_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_ROOT_SIGNATURE_INCLUDE_JHGSD

#include "DX12RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

// Mesh 管线的根签名需要三组可见性：D3D12 不允许对 amplification / mesh 阶段使用
// SHADER_VISIBILITY_ALL，而 Mesh 管线同时包含 AS / MS / PS，三者都要看到资源。
// 布局：0..3 = AMPLIFICATION（供 AS 使用）、4..7 = MESH、8..11 = PIXEL。
constexpr uint32_t GNX_DX12_MESH_ROOT_PARAM_AMPLIFICATION_OFFSET = 0;
constexpr uint32_t GNX_DX12_MESH_ROOT_PARAM_MESH_OFFSET          = 4;
constexpr uint32_t GNX_DX12_MESH_ROOT_PARAM_PIXEL_OFFSET         = 8;
constexpr uint32_t GNX_DX12_MESH_ROOT_PARAM_GROUPS               = 3;

class DX12RootSignature
{
public:
    DX12RootSignature() = default;
    ~DX12RootSignature();

    DX12RootSignature(const DX12RootSignature&) = delete;
    DX12RootSignature& operator=(const DX12RootSignature&) = delete;

    /// 一次性创建全部根签名变体
    bool Init(ID3D12Device* device);

    void Destroy();

    bool IsValid() const { return mGraphicsRootSignature != nullptr; }

    /// 图形管线（传统 VS+PS）使用的根签名
    ID3D12RootSignature* GetGraphicsRootSignature() const { return mGraphicsRootSignature.Get(); }

    /// 计算管线使用的根签名
    ID3D12RootSignature* GetComputeRootSignature() const { return mComputeRootSignature.Get(); }

    /// Mesh 管线使用的根签名；设备不支持 Mesh Shader 时为 nullptr
    ID3D12RootSignature* GetMeshRootSignature() const { return mMeshRootSignature.Get(); }

    bool HasMeshRootSignature() const { return mMeshRootSignature != nullptr; }

private:
    bool CreateRootSignature(ID3D12Device* device, bool isMesh, bool allowInputAssembler,
                             const wchar_t* debugName, ComPtr<ID3D12RootSignature>& outSignature);

    ComPtr<ID3D12RootSignature> mGraphicsRootSignature;
    ComPtr<ID3D12RootSignature> mComputeRootSignature;
    ComPtr<ID3D12RootSignature> mMeshRootSignature;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_ROOT_SIGNATURE_INCLUDE_JHGSD */
