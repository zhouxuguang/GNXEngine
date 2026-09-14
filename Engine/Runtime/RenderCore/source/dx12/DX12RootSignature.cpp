//
//  DX12RootSignature.cpp
//  rendercore
//

#include "DX12RootSignature.h"

NAMESPACE_RENDERCORE_BEGIN

namespace
{
// 描述符表的范围定义（与 HLSL 寄存器一一对应）
const D3D12_DESCRIPTOR_RANGE_TYPE kTableRangeTypes[GNX_DX12_ROOT_PARAM_COUNT] = {
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
};

const uint32_t kTableWidths[GNX_DX12_ROOT_PARAM_COUNT] = {
    GNX_DX12_CBV_TABLE_WIDTH,
    GNX_DX12_SRV_TABLE_WIDTH,
    GNX_DX12_UAV_TABLE_WIDTH,
    GNX_DX12_SAMPLER_TABLE_WIDTH,
};
} // namespace

DX12RootSignature::~DX12RootSignature()
{
    Destroy();
}

void DX12RootSignature::Destroy()
{
    mMeshRootSignature.Reset();
    mComputeRootSignature.Reset();
    mGraphicsRootSignature.Reset();
}

bool DX12RootSignature::CreateRootSignature(ID3D12Device* device, bool isMesh,
                                            bool allowInputAssembler, const wchar_t* debugName,
                                            ComPtr<ID3D12RootSignature>& outSignature)
{
    if (device == nullptr)
    {
        return false;
    }

    // 每个根签名最多 12 张表：非 Mesh 用前 4 张（ALL 可见）；
    // Mesh 用 12 张（AMPLIFICATION 4 张 + MESH 4 张 + PIXEL 4 张）
    const uint32_t tableSetCount = isMesh ? GNX_DX12_MESH_ROOT_PARAM_GROUPS : 1u;
    const uint32_t rootParamCount = GNX_DX12_ROOT_PARAM_COUNT * tableSetCount;

    // 数组长度必须是编译期常量，因此按最大可能长度（Mesh 变体）固定
    constexpr uint32_t kMaxRootParamCount = GNX_DX12_ROOT_PARAM_COUNT * GNX_DX12_MESH_ROOT_PARAM_GROUPS;
    D3D12_DESCRIPTOR_RANGE ranges[kMaxRootParamCount] = {};
    D3D12_ROOT_PARAMETER  rootParams[kMaxRootParamCount] = {};

    for (uint32_t set = 0; set < tableSetCount; ++set)
    {
        // Mesh 管线：set 0 = AMPLIFICATION（AS），set 1 = MESH（MS），set 2 = PIXEL（PS）
        // 普通图形管线：单一 ALL 可见性
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
        if (isMesh)
        {
            switch (set)
            {
                case 0:  visibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION; break;
                case 1:  visibility = D3D12_SHADER_VISIBILITY_MESH;          break;
                default: visibility = D3D12_SHADER_VISIBILITY_PIXEL;         break;
            }
        }

        for (uint32_t i = 0; i < GNX_DX12_ROOT_PARAM_COUNT; ++i)
        {
            D3D12_DESCRIPTOR_RANGE& range = ranges[set * GNX_DX12_ROOT_PARAM_COUNT + i];
            range.RangeType                         = kTableRangeTypes[i];
            range.NumDescriptors                    = kTableWidths[i];
            range.BaseShaderRegister                = 0;   // 从 registerX0 开始
            range.RegisterSpace                     = 0;
            // 每张表只含一个范围，且由编码器独立绑定基址，因此表内偏移恒为 0。
            range.OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER& param = rootParams[set * GNX_DX12_ROOT_PARAM_COUNT + i];
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.DescriptorTable.NumDescriptorRanges = 1;
            param.DescriptorTable.pDescriptorRanges = &range;
            param.ShaderVisibility = visibility;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = rootParamCount;
    rootSignatureDesc.pParameters = rootParams;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    // 图形管线需要输入装配；计算与 Mesh 则不需要（比流水线状态更严格，避免误用）
    if (allowInputAssembler)
    {
        rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    ComPtr<ID3DBlob> serializedBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &serializedBlob, &errorBlob);
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] D3D12SerializeRootSignature failed: %s | %s",
                  DX12HResultToString(hr),
                  errorBlob ? (const char*)errorBlob->GetBufferPointer() : "(no detail)");
        return false;
    }

    hr = device->CreateRootSignature(0, serializedBlob->GetBufferPointer(),
                                     serializedBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&outSignature));
    if (FAILED(hr) || !outSignature)
    {
        LOG_ERROR("[DX12] CreateRootSignature failed: %s | %s", DX12HResultToString(hr),
                  errorBlob ? (const char*)errorBlob->GetBufferPointer() : "(no detail)");
        return false;
    }

    outSignature->SetName(debugName);
    return true;
}

bool DX12RootSignature::Init(ID3D12Device* device)
{
    if (device == nullptr)
    {
        return false;
    }

    Destroy();

    if (!CreateRootSignature(device, /*isMesh=*/false, /*allowInputAssembler=*/true,
                             L"DX12 Graphics Root Signature", mGraphicsRootSignature))
    {
        return false;
    }

    if (!CreateRootSignature(device, /*isMesh=*/false, /*allowInputAssembler=*/false,
                             L"DX12 Compute Root Signature", mComputeRootSignature))
    {
        return false;
    }

    // Mesh 根签名是可选能力：不支持 Mesh Shader 的设备上允许创建失败
    if (!CreateRootSignature(device, /*isMesh=*/true, /*allowInputAssembler=*/false,
                             L"DX12 Mesh Root Signature", mMeshRootSignature))
    {
        LOG_WARN("[DX12] Mesh root signature creation failed; mesh pipelines will be unavailable.");
        mMeshRootSignature.Reset();
    }

    return true;
}

NAMESPACE_RENDERCORE_END
