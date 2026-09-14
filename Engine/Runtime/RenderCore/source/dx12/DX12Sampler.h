//
//  DX12Sampler.h
//  rendercore
//
//  D3D12 采样器实现。
//
//  与 Vulkan 不同，D3D12 的采样器不是独立对象，而是描述符堆里的一条记录。
//  因此本类只保存 SamplerDesc 的换算结果，由编码器在绑定时写入命令缓冲区的
//  采样器环形堆。
//

#ifndef GNX_ENGINE_DX12_SAMPLER_INCLUDE_JHGSDF
#define GNX_ENGINE_DX12_SAMPLER_INCLUDE_JHGSDF

#include "DX12RenderDefine.h"
#include "DX12Util.h"
#include "TextureSampler.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12TextureSampler : public TextureSampler
{
public:
    DX12TextureSampler(const SamplerDesc& des);
    ~DX12TextureSampler() override = default;

    const D3D12_SAMPLER_DESC& GetSamplerDesc() const { return mSamplerDesc; }

    /// 把采样器写入指定的描述符位置
    void WriteSampler(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE dest) const;

    const SamplerDesc& GetEngineDesc() const { return mEngineDesc; }

private:
    SamplerDesc mEngineDesc;
    D3D12_SAMPLER_DESC mSamplerDesc = {};
};

using DX12TextureSamplerPtr = std::shared_ptr<DX12TextureSampler>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_SAMPLER_INCLUDE_JHGSDF */
