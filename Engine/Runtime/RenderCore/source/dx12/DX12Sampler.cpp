//
//  DX12Sampler.cpp
//  rendercore
//

#include "DX12Sampler.h"

NAMESPACE_RENDERCORE_BEGIN

DX12TextureSampler::DX12TextureSampler(const SamplerDesc& des)
    : TextureSampler(des)
    , mEngineDesc(des)
{
    DX12Util::FillSamplerDesc(des, mSamplerDesc);
}

void DX12TextureSampler::WriteSampler(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest) const
{
    if (device == nullptr)
    {
        return;
    }
    device->CreateSampler(&mSamplerDesc, dest);
}

NAMESPACE_RENDERCORE_END
