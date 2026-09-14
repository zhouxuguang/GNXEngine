//
//  DX12ComputeEncoder.cpp
//  rendercore
//

#include "DX12ComputeEncoder.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"

NAMESPACE_RENDERCORE_BEGIN

namespace
{
} // namespace

DX12ComputeEncoder::DX12ComputeEncoder(const DX12CommandBufferPtr& commandBuffer)
    : mCommandBuffer(commandBuffer)
{
    if (!mCommandBuffer || mCommandBuffer->GetCommandList() == nullptr)
    {
        return;
    }
    mCommandList = mCommandBuffer->GetCommandList();
    mContext = mCommandBuffer->GetContext().get();
    mEncoding = true;
}

DX12ComputeEncoder::~DX12ComputeEncoder()
{
    if (mEncoding)
    {
        EndEncode();
    }
}

void DX12ComputeEncoder::EndEncode()
{
    mEncoding = false;
}

bool DX12ComputeEncoder::EnsureBlock()
{
    if (!mComputePipeline)
    {
        return false;
    }
    return mCommandBuffer->AcquireComputeDescriptorBlock(mComputePipeline);
}

const DX12BindInfo* DX12ComputeEncoder::ResolveBinding(const std::string& name) const
{
    const DX12BindInfo* info = mComputePipeline ? mComputePipeline->FindBinding(name) : nullptr;

    return info;
}

void DX12ComputeEncoder::SetComputePipeline(ComputePipelinePtr computePipeline)
{
    auto pipeline = std::dynamic_pointer_cast<DX12ComputePipeline>(computePipeline);
    if (!pipeline)
    {
        return;
    }

    mComputePipeline = pipeline.get();
    mCommandBuffer->SetCurrentComputePipeline(mComputePipeline);
    mCommandBuffer->SetCurrentRootSignature(pipeline->GetRootSignature()->GetComputeRootSignature(),
                                            false);

    if (ID3D12PipelineState* pso = pipeline->GetPipelineState())
    {
        mCommandList->SetPipelineState(pso);
        mCommandList->SetComputeRootSignature(pipeline->GetRootSignature()->GetComputeRootSignature());
    }

    mCommandBuffer->AcquireComputeDescriptorBlock(mComputePipeline);
}

void DX12ComputeEncoder::WriteCBV(uint32_t reg, UniformBufferPtr buffer)
{
    auto uniformBuffer = std::dynamic_pointer_cast<DX12UniformBuffer>(buffer);
    if (!uniformBuffer || !EnsureBlock())
    {
        return;
    }

    const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
        DX12CBVDesc(uniformBuffer->GetGPUAddress(), uniformBuffer->GetAlignedSize());
    mContext->device->CreateConstantBufferView(
        &cbvDesc, mCommandBuffer->GetCpuHandle(DX12DescriptorClass::CBV, reg));
}

void DX12ComputeEncoder::TrackUAVResource(ID3D12Resource* resource)
{
    if (resource == nullptr)
    {
        return;
    }
    for (const auto& existing : mPendingUAVBarriers)
    {
        if (existing.Get() == resource)
        {
            return;
        }
    }
    mPendingUAVBarriers.push_back(resource);
}

void DX12ComputeEncoder::WriteSRVBuffer(uint32_t reg, RCBufferPtr buffer, bool asUAV,
                                        bool isRawBuffer, uint32_t structuredStride)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!dx12Buffer || !EnsureBlock())
    {
        return;
    }

    const bool created = DX12CreateBufferView(
        mContext->device.Get(), dx12Buffer->GetResource(), dx12Buffer->GetSizeInBytes(),
        asUAV, isRawBuffer, structuredStride,
        mCommandBuffer->GetCpuHandle(
            asUAV ? DX12DescriptorClass::UAV : DX12DescriptorClass::SRV, reg));

    if (created && asUAV)
    {
        TrackUAVResource(dx12Buffer->GetResource());
    }
}

void DX12ComputeEncoder::WriteSRVTexture(uint32_t reg, RCTexturePtr texture, uint32_t mipLevel,
                                        bool asUAV)
{
    auto dx12Texture = std::dynamic_pointer_cast<DX12TextureBase>(texture);
    if (!dx12Texture || !EnsureBlock())
    {
        return;
    }

    if (asUAV)
    {
        dx12Texture->WriteUAV(mContext->device.Get(),
                              mCommandBuffer->GetCpuHandle(DX12DescriptorClass::UAV, reg), mipLevel);
        TrackUAVResource(dx12Texture->GetResource());
    }
    else
    {
        dx12Texture->WriteSRV(mContext->device.Get(),
                             mCommandBuffer->GetCpuHandle(DX12DescriptorClass::SRV, reg), mipLevel);
    }
}

// ---- UniformBuffer ----

void DX12ComputeEncoder::SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr || binding->cls != DX12DescriptorClass::CBV)
    {
        return;
    }
    WriteCBV(binding->bindPoint, buffer);
}

// ---- StorageBuffer ----

void DX12ComputeEncoder::SetStorageBuffer(RCBufferPtr buffer, uint32_t index)
{
    // 按索引绑定时，index 直接解释为 HLSL 寄存器号（与 Vulkan 后端的语义一致）。
    // 由于需要 raw / 步长标志才能正确创建视图，这里按寄存器反查反射信息。
    const DX12ShaderFunctionPtr shader = mComputePipeline ? mComputePipeline->GetShader() : nullptr;
    if (!shader)
    {
        LOG_ERROR("[DX12] SetStorageBuffer(%u) 在未绑定计算管线时调用，已跳过", index);
        return;
    }

    const DX12BindInfo* info = shader->FindBindingByRegister(DX12DescriptorClass::UAV, index);
    bool asUAV = (info != nullptr);
    if (info == nullptr)
    {
        info = shader->FindBindingByRegister(DX12DescriptorClass::SRV, index);
    }
    if (info == nullptr)
    {
        LOG_ERROR("[DX12] SetStorageBuffer: 寄存器 u%u/t%u 在计算着色器中不存在，已跳过",
                  index, index);
        return;
    }

    WriteSRVBuffer(index, buffer, asUAV, info->isRawBuffer, info->structuredStride);
}

void DX12ComputeEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr)
    {
        return;
    }
    const bool asUAV = (binding->cls == DX12DescriptorClass::UAV);
    if (!asUAV && binding->cls != DX12DescriptorClass::SRV)
    {
        return;
    }
    WriteSRVBuffer(binding->bindPoint, buffer, asUAV, binding->isRawBuffer,
                   binding->structuredStride);
}

// ---- 输入纹理 ----

void DX12ComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t index)
{
    WriteSRVTexture(index, texture, DX12_ALL_MIPS, false);
}

void DX12ComputeEncoder::SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    WriteSRVTexture(index, texture, mipLevel, false);
}

void DX12ComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr || binding->cls != DX12DescriptorClass::SRV)
    {
        return;
    }
    WriteSRVTexture(binding->bindPoint, texture, DX12_ALL_MIPS, false);
}

void DX12ComputeEncoder::SetTexture(const std::string& resourceName, RCTexturePtr texture,
                                    uint32_t mipLevel)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr || binding->cls != DX12DescriptorClass::SRV)
    {
        return;
    }
    WriteSRVTexture(binding->bindPoint, texture, mipLevel, false);
}

// ---- 输出纹理（UAV）----

void DX12ComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t index)
{
    WriteSRVTexture(index, texture, 0, true);
}

void DX12ComputeEncoder::SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index)
{
    WriteSRVTexture(index, texture, mipLevel, true);
}

void DX12ComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr || binding->cls != DX12DescriptorClass::UAV)
    {
        return;
    }
    WriteSRVTexture(binding->bindPoint, texture, 0, true);
}

void DX12ComputeEncoder::SetOutTexture(const std::string& resourceName, RCTexturePtr texture,
                                       uint32_t mipLevel)
{
    const DX12BindInfo* binding = ResolveBinding(resourceName);
    if (binding == nullptr || binding->cls != DX12DescriptorClass::UAV)
    {
        return;
    }
    WriteSRVTexture(binding->bindPoint, texture, mipLevel, true);
}

void DX12ComputeEncoder::Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY,
                                  uint32_t threadGroupsZ)
{
    if (!mCommandList || !mComputePipeline)
    {
        return;
    }

    // 上一轮 dispatch 写入的 UAV 与本轮之间需要 UAV barrier（RAW/WAW 保护）。
    // 引擎也会显式调用 ResourceBarrier，这里做兜底避免漏掉。
    if (!mPendingUAVBarriers.empty())
    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(mPendingUAVBarriers.size());
        for (const auto& resource : mPendingUAVBarriers)
        {
            barriers.push_back(DX12UAVBarrier(resource.Get()));
        }
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
        mPendingUAVBarriers.clear();
    }

    mCommandBuffer->BindComputeDescriptorTables();
    mCommandList->Dispatch(threadGroupsX, threadGroupsY, threadGroupsZ);
    mCommandBuffer->MarkBlockUsedByDraw();
}

NAMESPACE_RENDERCORE_END
