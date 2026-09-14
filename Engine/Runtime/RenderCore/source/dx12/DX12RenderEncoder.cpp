//
//  DX12RenderEncoder.cpp
//  rendercore
//

#include "DX12RenderEncoder.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12Sampler.h"
#include "DX12Util.h"

NAMESPACE_RENDERCORE_BEGIN

namespace
{
/**
 * @brief 遍历某个资源名在各 stage 组中的绑定
 *
 * ── 为什么要按 stage 分开绑定 ──
 * HLSL 的 b/t/u/s 寄存器是**每个 stage 独立分配**的：同一个资源名在
 * amplification shader 里可能是 t0，在 mesh shader 里却是 t1（各 stage 用到的
 * 资源集合不同，DXC 按各自的声明顺序编号）。因此 Mesh 管线不能像普通管线那样
 * 用一份"合并绑定表"，必须逐 stage 用该 stage 自己的寄存器解析，并写入对应的
 * stage 组（组布局见 DX12RenderDefine.h）。
 *
 * 调用方只需按名字绑定一次，本函数会把描述符写进所有真正使用该资源的 stage，
 * 这样 SetMeshUniformBuffer / SetTaskUniformBuffer 等入口的行为都一致且幂等。
 *
 * 非 Mesh 管线只有第 0 组，取合并结果 —— 与历史行为完全一致。
 */
template <typename Fn>
void ForEachStageBinding(DX12GraphicsPipeline* pipeline, const std::string& name, Fn&& fn)
{
    if (pipeline == nullptr)
    {
        return;
    }

    if (!pipeline->IsMeshPipeline())
    {
        const DX12BindInfo* info = pipeline->FindBinding(name);
        if (info != nullptr)
        {
            fn(0u, *info);
        }
        return;
    }

    static const ShaderStage kStageOrder[GNX_DX12_STAGE_GROUP_MAX] = {
        ShaderStage_Task, ShaderStage_Mesh, ShaderStage_Fragment
    };
    for (uint32_t group = 0; group < GNX_DX12_STAGE_GROUP_MAX; ++group)
    {
        const DX12BindInfo* info = pipeline->FindBinding(name, kStageOrder[group]);
        if (info != nullptr)
        {
            fn(group, *info);
        }
    }
}
} // namespace

// ============================================================================
// Create raw or structured SRV/UAV views from reflected binding metadata.
// ============================================================================

// ============================================================================
// 构造 / 析构
// ============================================================================

DX12RenderEncoder::DX12RenderEncoder(const DX12CommandBufferPtr& commandBuffer,
                                     const RenderPass& renderPass)
    : mCommandBuffer(commandBuffer)
{
    if (!mCommandBuffer || mCommandBuffer->GetCommandList() == nullptr)
    {
        return;
    }

    mCommandList = mCommandBuffer->GetCommandList();
    mContext = mCommandBuffer->GetContext().get();
    mEncoding = true;

    BuildTargetsFromRenderPass(renderPass);
    BeginTargets();
}

DX12RenderEncoder::DX12RenderEncoder(const DX12CommandBufferPtr& commandBuffer,
                                     const ClearColor& clearColor,
                                     bool useSwapChainTargets)
    : mCommandBuffer(commandBuffer)
    , mIsSwapChainPass(useSwapChainTargets)
{
    if (!mCommandBuffer || mCommandBuffer->GetCommandList() == nullptr)
    {
        return;
    }

    mCommandList = mCommandBuffer->GetCommandList();
    mContext = mCommandBuffer->GetContext().get();
    mEncoding = true;

    // 默认编码器：上屏。忽略 useSwapChainTargets（false 时按无附件处理）
    if (useSwapChainTargets && mCommandBuffer->GetSwapChain())
    {
        BuildSwapChainTargets(clearColor);
    }
    else
    {
        // 无渲染目标的空 pass（等价于 Vulkan 的空 renderpass）
        mTargets.width = 1;
        mTargets.height = 1;
    }

    BeginTargets();
}

DX12RenderEncoder::~DX12RenderEncoder()
{
    if (mEncoding)
    {
        EndEncode();
    }
}

// ============================================================================
// 渲染目标
// ============================================================================

void DX12RenderEncoder::BuildTargetsFromRenderPass(const RenderPass& renderPass)
{
    mTargets.colorTextures.clear();
    mTargets.colorRTVs.clear();
    mTargets.colorClearRTVs.clear();

    for (const auto& attachment : renderPass.colorAttachments)
    {
        if (!attachment || !attachment->texture)
        {
            continue;
        }

        auto texture = std::dynamic_pointer_cast<DX12TextureBase>(attachment->texture);
        if (!texture || !texture->IsValid())
        {
            LOG_WARN("[DX12] RenderPass color attachment is not a valid DX12 texture, skipped");
            continue;
        }

        mTargets.colorTextures.push_back(texture);
        mTargets.colorRTVs.push_back(texture->GetRTVHandle(attachment->level, attachment->slice));

        if (attachment->loadOp == ATTACHMENT_LOAD_OP_CLEAR)
        {
            // clear 颜色需要逐附件记录（D3D12 的 Clear 按单个 RTV 进行）
            // 这里用额外的 RTV 数组按索引对应；实际 clear 值在调用点展开
            mTargets.colorClearRTVs.push_back(mTargets.colorRTVs.back());
            mTargets.colorClearColors.push_back({attachment->clearColor.red, attachment->clearColor.green,
                                                 attachment->clearColor.blue, attachment->clearColor.alpha});
        }

        mTargets.psoFormat.colorFormats.push_back(texture->GetDXGIFormat());
    }

    if (renderPass.depthAttachment && renderPass.depthAttachment->texture)
    {
        auto depthTexture = std::dynamic_pointer_cast<DX12TextureBase>(renderPass.depthAttachment->texture);
        if (depthTexture && depthTexture->IsValid())
        {
            mTargets.depthTexture = depthTexture;
            mTargets.hasDepth = true;
            mTargets.depthReadOnly = renderPass.depthAttachment->readOnly;
            mTargets.depthDSV = depthTexture->GetDSVHandle();
            mTargets.psoFormat.depthFormat = depthTexture->GetDXGIFormat();

            if (renderPass.depthAttachment->loadOp == ATTACHMENT_LOAD_OP_CLEAR)
            {
                mTargets.clearDepth = true;
                mTargets.clearDepthValue = renderPass.depthAttachment->clearDepth;
            }
        }
    }

    // 渲染区域
    const uint32_t w = (renderPass.renderRegion.width > 0)
                           ? (uint32_t)renderPass.renderRegion.width : 1u;
    const uint32_t h = (renderPass.renderRegion.height > 0)
                           ? (uint32_t)renderPass.renderRegion.height : 1u;
    mTargets.width = w;
    mTargets.height = h;
    mTargets.psoFormat.sampleCount = 1;
}

void DX12RenderEncoder::BuildSwapChainTargets(const ClearColor& clearColor)
{
    const DX12SwapChainPtr& swapChain = mCommandBuffer->GetSwapChain();
    if (!swapChain)
    {
        return;
    }

    const auto& backBuffer = swapChain->GetCurrentBackBufferTexture();
    const auto& depthTexture = swapChain->GetCurrentDepthTexture();

    if (backBuffer && backBuffer->IsValid())
    {
        float cr = clearColor.red, cg = clearColor.green, cb = clearColor.blue, ca = clearColor.alpha;

        mTargets.colorTextures.push_back(backBuffer);
        mTargets.colorRTVs.push_back(backBuffer->GetRTVHandle());
        mTargets.colorClearRTVs.push_back(mTargets.colorRTVs.back());
        mTargets.colorClearColors.push_back({cr, cg, cb, ca});
        mTargets.psoFormat.colorFormats.push_back(backBuffer->GetDXGIFormat());
    }

    if (depthTexture && depthTexture->IsValid())
    {
        mTargets.depthTexture = depthTexture;
        mTargets.hasDepth = true;
        mTargets.depthReadOnly = false;
        mTargets.depthDSV = depthTexture->GetDSVHandle();
        mTargets.clearDepth = true;
        mTargets.clearDepthValue = DepthConfig::GetDefaultClearDepth();
        mTargets.psoFormat.depthFormat = depthTexture->GetDXGIFormat();
    }

    mTargets.width = swapChain->GetWidth();
    mTargets.height = swapChain->GetHeight();
    mTargets.psoFormat.sampleCount = 1;
}

void DX12RenderEncoder::BeginTargets()
{
    if (!mCommandList)
    {
        return;
    }

    // ---- 状态转换 ----
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    for (const auto& texture : mTargets.colorTextures)
    {
        if (texture->GetCurrentState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            barriers.push_back(DX12TransitionBarrier(texture->GetResource(),
                                                     texture->GetCurrentState(),
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET));
            texture->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
    }

    // 只读深度：v1 简化为仍绑定可写 DSV + DEPTH_WRITE 状态。
    // 正确性依据：PSO 的 DepthWriteMask=ZERO 阻止实际写入，内容不会破坏。
    if (mTargets.hasDepth && mTargets.depthTexture)
    {
        if (mTargets.depthTexture->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            barriers.push_back(DX12TransitionBarrier(mTargets.depthTexture->GetResource(),
                                                     mTargets.depthTexture->GetCurrentState(),
                                                     D3D12_RESOURCE_STATE_DEPTH_WRITE));
            mTargets.depthTexture->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }
    }

    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }

    // ---- 绑定渲染目标 ----
    mCommandList->OMSetRenderTargets(
        (UINT)mTargets.colorRTVs.size(),
        mTargets.colorRTVs.empty() ? nullptr : mTargets.colorRTVs.data(),
        FALSE,
        mTargets.hasDepth ? &mTargets.depthDSV : nullptr);

    // ---- 视口与裁剪（正向高度，见 DX12Pipeline 对 FrontCounterClockwise 的说明）----
    const D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)mTargets.width, (float)mTargets.height,
                                      0.0f, 1.0f };
    mCommandList->RSSetViewports(1, &viewport);

    const D3D12_RECT scissor = { 0, 0, (LONG)mTargets.width, (LONG)mTargets.height };
    mCommandList->RSSetScissorRects(1, &scissor);

    // ---- Clear（loadOp == CLEAR 的附件）----
    for (size_t i = 0; i < mTargets.colorClearRTVs.size() && i < mTargets.colorClearColors.size(); ++i)
    {
        mCommandList->ClearRenderTargetView(mTargets.colorClearRTVs[i],
                                            mTargets.colorClearColors[i].data(), 1, &scissor);
    }

    if (mTargets.clearDepth && mTargets.hasDepth)
    {
        mCommandList->ClearDepthStencilView(mTargets.depthDSV,
                                            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                            mTargets.clearDepthValue, mTargets.clearStencilValue,
                                            1, &scissor);
    }
}

void DX12RenderEncoder::EndTargets()
{
    if (!mCommandList)
    {
        return;
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    // 上屏 pass → PRESENT；离屏 pass → ALL_SHADER_RESOURCE（供后续 pass 采样）
    const D3D12_RESOURCE_STATES colorFinal = mIsSwapChainPass
                                                 ? D3D12_RESOURCE_STATE_PRESENT
                                                 : D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

    for (const auto& texture : mTargets.colorTextures)
    {
        if (texture->GetCurrentState() != colorFinal)
        {
            barriers.push_back(DX12TransitionBarrier(texture->GetResource(),
                                                     texture->GetCurrentState(), colorFinal));
            texture->SetCurrentState(colorFinal);
        }
    }

    // 深度 → DEPTH_READ：FrameGraph 的后续 pass 通常会采样深度（SSAO/HiZ/SSR）
    if (mTargets.hasDepth && mTargets.depthTexture)
    {
        if (mTargets.depthTexture->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_READ)
        {
            barriers.push_back(DX12TransitionBarrier(mTargets.depthTexture->GetResource(),
                                                     mTargets.depthTexture->GetCurrentState(),
                                                     D3D12_RESOURCE_STATE_DEPTH_READ));
            mTargets.depthTexture->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_READ);
        }
    }

    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier((UINT)barriers.size(), barriers.data());
    }
}

void DX12RenderEncoder::EndEncode()
{
    if (!mEncoding)
    {
        return;
    }
    mEncoding = false;
    EndTargets();
}

// ============================================================================
// 管线与状态
// ============================================================================

void DX12RenderEncoder::SetGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline)
{
    auto pipeline = std::dynamic_pointer_cast<DX12GraphicsPipeline>(graphicsPipeline);
    if (!pipeline)
    {
        return;
    }

    mGraphicsPipeline = pipeline.get();
    mCurrentFillMode = pipeline->GetDesc().fillMode;
    mDepthBiasUnits = 0.0f;
    mDepthBiasSlope = 0.0f;

    // 记录当前根签名（Mesh 管线用 MESH 可见的变体）
    DX12RootSignature* rootSignature = pipeline->GetRootSignature();
    ID3D12RootSignature* d3dRootSignature = mGraphicsPipeline->IsMeshPipeline()
                                                ? rootSignature->GetMeshRootSignature()
                                                : rootSignature->GetGraphicsRootSignature();
    mCommandBuffer->SetCurrentRootSignature(d3dRootSignature, mGraphicsPipeline->IsMeshPipeline());
    mCommandBuffer->SetCurrentGraphicsPipeline(mGraphicsPipeline);

    BindPipelineIfNeeded();
    mCommandBuffer->AcquireDescriptorBlock(mGraphicsPipeline);
}

void DX12RenderEncoder::BindPipelineIfNeeded()
{
    if (!mGraphicsPipeline || !mCommandList)
    {
        return;
    }

    ID3D12PipelineState* pso = mGraphicsPipeline->GetPipelineState(mTargets.psoFormat, CurrentVariantKey());
    if (pso == nullptr)
    {
        return;
    }

    mCommandList->SetPipelineState(pso);
}

void DX12RenderEncoder::SetFillMode(FillMode fillMode)
{
    if (mCurrentFillMode == fillMode)
    {
        return;
    }
    mCurrentFillMode = fillMode;
    // fillMode 是 PSO 静态状态：切换变体并重新绑定
    if (mGraphicsPipeline)
    {
        BindPipelineIfNeeded();
    }
}

void DX12RenderEncoder::SetDepthBias(float bias, float slopeScale, float clamp)
{
    // clamp 在 D3D12_RASTERIZER_DESC 里是 DepthBiasClamp；引擎不区分，记录并忽略
    (void)clamp;
    mDepthBiasUnits = bias;
    mDepthBiasSlope = slopeScale;
    if (mGraphicsPipeline)
    {
        BindPipelineIfNeeded();
    }
}

void DX12RenderEncoder::SetStencilReference(uint32_t frontRef, uint32_t backRef)
{
    if (!mCommandList)
    {
        return;
    }
    // D3D12 只支持单一模板参考值（正反面共用）；引擎两侧通常一致
    (void)backRef;
    mCommandList->OMSetStencilRef(frontRef);
}

void DX12RenderEncoder::SetScissorRect(int x, int y, uint32_t width, uint32_t height)
{
    if (!mCommandList)
    {
        return;
    }
    const D3D12_RECT rect = { (LONG)x, (LONG)y, (LONG)(x + width), (LONG)(y + height) };
    mCommandList->RSSetScissorRects(1, &rect);
}

// ============================================================================
// 描述符绑定
// ============================================================================

bool DX12RenderEncoder::EnsureBlock()
{
    if (!mGraphicsPipeline)
    {
        return false;
    }
    return mCommandBuffer->AcquireDescriptorBlock(mGraphicsPipeline);
}

uint32_t DX12RenderEncoder::StageGroup(ShaderStage stage) const
{
    // 非 Mesh 管线的根签名只有一组 SHADER_VISIBILITY_ALL 的表，
    // 所有 stage 都从第 0 组取资源。
    if (!mGraphicsPipeline || !mGraphicsPipeline->IsMeshPipeline())
    {
        return 0;
    }
    return DX12StageGroupIndex(stage);
}

const DX12BindInfo* DX12RenderEncoder::ResolveBinding(const std::string& name,
                                                      const ShaderStage* stage) const
{
    const DX12BindInfo* info = nullptr;
    if (mGraphicsPipeline)
    {
        info = (stage != nullptr) ? mGraphicsPipeline->FindBinding(name, *stage)
                                  : mGraphicsPipeline->FindBinding(name);
    }

    return info;
}

void DX12RenderEncoder::WriteCBV(uint32_t registerIndex, UniformBufferPtr buffer,
                                 uint32_t stageGroup)
{
    auto uniformBuffer = std::dynamic_pointer_cast<DX12UniformBuffer>(buffer);
    if (!uniformBuffer || !EnsureBlock())
    {
        return;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE handle =
        mCommandBuffer->GetCpuHandle(DX12DescriptorClass::CBV, registerIndex, stageGroup);
    const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc =
        DX12CBVDesc(uniformBuffer->GetGPUAddress(), uniformBuffer->GetAlignedSize());
    mContext->device->CreateConstantBufferView(&cbvDesc, handle);
}

void DX12RenderEncoder::WriteSRVTexture(uint32_t registerIndex, RCTexturePtr texture,
                                        uint32_t stageGroup)
{
    auto dx12Texture = std::dynamic_pointer_cast<DX12TextureBase>(texture);
    if (!dx12Texture || !EnsureBlock())
    {
        return;
    }
    dx12Texture->WriteSRV(mContext->device.Get(),
                          mCommandBuffer->GetCpuHandle(DX12DescriptorClass::SRV, registerIndex,
                                                       stageGroup));
}

void DX12RenderEncoder::WriteUAVTexture(uint32_t registerIndex, RCTexturePtr texture,
                                        uint32_t stageGroup)
{
    auto dx12Texture = std::dynamic_pointer_cast<DX12TextureBase>(texture);
    if (!dx12Texture || !EnsureBlock())
    {
        return;
    }
    dx12Texture->WriteUAV(mContext->device.Get(),
                          mCommandBuffer->GetCpuHandle(DX12DescriptorClass::UAV, registerIndex,
                                                       stageGroup));
}

void DX12RenderEncoder::WriteSRVBuffer(uint32_t registerIndex, RCBufferPtr buffer, bool asUAV,
                                       bool isRawBuffer, uint32_t structuredStride,
                                       uint32_t stageGroup)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!dx12Buffer || !EnsureBlock())
    {
        return;
    }

    DX12CreateBufferView(mContext->device.Get(), dx12Buffer->GetResource(),
                         dx12Buffer->GetSizeInBytes(), asUAV, isRawBuffer, structuredStride,
                         mCommandBuffer->GetCpuHandle(
                             asUAV ? DX12DescriptorClass::UAV : DX12DescriptorClass::SRV,
                             registerIndex, stageGroup));
}

void DX12RenderEncoder::WriteSampler(uint32_t registerIndex, TextureSamplerPtr sampler,
                                     uint32_t stageGroup)
{
    auto dx12Sampler = std::dynamic_pointer_cast<DX12TextureSampler>(sampler);
    if (!dx12Sampler || !EnsureBlock())
    {
        return;
    }
    dx12Sampler->WriteSampler(mContext->device.Get(),
        mCommandBuffer->GetCpuHandle(DX12DescriptorClass::Sampler, registerIndex, stageGroup));
}

void DX12RenderEncoder::FlushDescriptorTables()
{
    if (!mGraphicsPipeline)
    {
        return;
    }
    mCommandBuffer->BindGraphicsDescriptorTables(mGraphicsPipeline->IsMeshPipeline());
}

// ---- Uniform Buffer ----

void DX12RenderEncoder::SetVertexUniformBuffer(UniformBufferPtr buffer, int index)
{
    WriteCBV((uint32_t)index, buffer);
}

void DX12RenderEncoder::SetFragmentUniformBuffer(UniformBufferPtr buffer, int index)
{
    // Vulkan 后端此重载为空操作（绑定走名字版本）；保持一致
    (void)buffer;
    (void)index;
}

void DX12RenderEncoder::SetVertexUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    ForEachStageBinding(mGraphicsPipeline, resourceName,
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        if (binding.cls == DX12DescriptorClass::CBV)
        {
            WriteCBV(binding.bindPoint, buffer, group);
        }
    });
}

void DX12RenderEncoder::SetFragmentUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    ForEachStageBinding(mGraphicsPipeline, resourceName,
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        if (binding.cls == DX12DescriptorClass::CBV)
        {
            WriteCBV(binding.bindPoint, buffer, group);
        }
    });
}

void DX12RenderEncoder::SetMeshUniformBuffer(UniformBufferPtr buffer, int index)
{
    // 按寄存器号绑定时无法按名字查表，只能落在调用方指定的 stage 组上
    WriteCBV((uint32_t)index, buffer, StageGroup(ShaderStage_Mesh));
}

void DX12RenderEncoder::SetTaskUniformBuffer(UniformBufferPtr buffer, int index)
{
    WriteCBV((uint32_t)index, buffer, StageGroup(ShaderStage_Task));
}

void DX12RenderEncoder::SetMeshUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    // 按名字绑定：由 ForEachStageBinding 覆盖所有真正用到它的 stage
    SetFragmentUniformBuffer(resourceName, buffer);
}

void DX12RenderEncoder::SetTaskUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer)
{
    SetFragmentUniformBuffer(resourceName, buffer);
}

// ---- SSBO ----

void DX12RenderEncoder::SetStorageBuffer(const std::string& resourceName, RCBufferPtr buffer, ShaderStage stage)
{
    // 与 Vulkan 后端一致：按名字解析到 SRV / UAV 槽位。
    // Mesh 管线必须逐 stage 解析（各 stage 的寄存器独立分配），因此
    // 这里把 stage 交给 ForEachStageBinding 统一处理，而不是只信一个槽位。
    (void)stage;
    ForEachStageBinding(mGraphicsPipeline, resourceName,
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        const bool asUAV = (binding.cls == DX12DescriptorClass::UAV);
        if (!asUAV && binding.cls != DX12DescriptorClass::SRV)
        {
            return;
        }
        WriteSRVBuffer(binding.bindPoint, buffer, asUAV, binding.isRawBuffer,
                       binding.structuredStride, group);
    });
}

void DX12RenderEncoder::SetFragmentStorageTexture(const std::string& resourceName, RCTexturePtr texture)
{
    ForEachStageBinding(mGraphicsPipeline, resourceName,
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        if (binding.cls == DX12DescriptorClass::UAV)
        {
            WriteUAVTexture(binding.bindPoint, texture, group);
        }
        else if (binding.cls == DX12DescriptorClass::SRV)
        {
            WriteSRVTexture(binding.bindPoint, texture, group);
        }
    });
}

// ---- 纹理与采样器 ----

void DX12RenderEncoder::SetFragmentTextureAndSampler(const std::string& resourceName,
                                                     RCTexturePtr texture,
                                                     TextureSamplerPtr sampler)
{
    // 纹理（SRV）：按 stage 分别写入各自寄存器
    ForEachStageBinding(mGraphicsPipeline, resourceName,
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        if (binding.cls == DX12DescriptorClass::SRV || binding.cls == DX12DescriptorClass::UAV)
        {
            WriteSRVTexture(binding.bindPoint, texture, group);
        }
    });

    // 采样器：与 Vulkan 后端的命名约定一致（resourceName + "Sam"）
    ForEachStageBinding(mGraphicsPipeline, resourceName + "Sam",
                        [&](uint32_t group, const DX12BindInfo& binding)
    {
        if (binding.cls == DX12DescriptorClass::Sampler)
        {
            WriteSampler(binding.bindPoint, sampler, group);
        }
    });
}

void DX12RenderEncoder::SetVertexTextureAndSampler(const std::string& resourceName,
                                                   RCTexturePtr texture, TextureSamplerPtr sampler)
{
    // D3D12 的表对所有阶段可见（ALL），与 Vulkan 的行为一致，直接委托
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

void DX12RenderEncoder::SetMeshTextureAndSampler(const std::string& resourceName,
                                                 RCTexturePtr texture, TextureSamplerPtr sampler)
{
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

void DX12RenderEncoder::SetTaskTextureAndSampler(const std::string& resourceName,
                                                 RCTexturePtr texture, TextureSamplerPtr sampler)
{
    SetFragmentTextureAndSampler(resourceName, texture, sampler);
}

// ============================================================================
// 顶点/索引缓冲
// ============================================================================

void DX12RenderEncoder::SetVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12VertexBuffer>(buffer);
    if (!dx12Buffer || !mCommandList)
    {
        return;
    }

    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = dx12Buffer->GetGPUAddress() + offset;
    view.SizeInBytes    = dx12Buffer->GetBufferLength() - offset;
    // stride 必须显式给出：D3D12 不会从 PSO 的输入布局推导（那是 Vulkan 的语义）。
    // 之前写死 0 会让 IA 反复读取同一个顶点，几何体退化成一条线，
    // 结果就是"带顶点缓冲的绘制全部不可见"，且不产生任何校验错误。
    view.StrideInBytes  = (mGraphicsPipeline != nullptr) ? mGraphicsPipeline->GetVertexStride((uint32_t)index) : 0;
    mCommandList->IASetVertexBuffers((UINT)index, 1, &view);
}

void DX12RenderEncoder::SetVertexBuffer(RCBufferPtr buffer, uint32_t offset, int index)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!dx12Buffer || !mCommandList)
    {
        return;
    }

    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = dx12Buffer->GetGPUAddress() + offset;
    view.SizeInBytes    = dx12Buffer->GetSizeInBytes() - offset;
    // 同上方重载：stride 必须显式给出，不能依赖 PSO 推导。
    view.StrideInBytes  = (mGraphicsPipeline != nullptr) ? mGraphicsPipeline->GetVertexStride((uint32_t)index) : 0;
    mCommandList->IASetVertexBuffers((UINT)index, 1, &view);
}

void DX12RenderEncoder::BindIndexBuffer(IndexBufferPtr buffer, int indexOffset)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12IndexBuffer>(buffer);
    if (!dx12Buffer || !mCommandList)
    {
        return;
    }

    const DXGI_FORMAT format = dx12Buffer->GetDXGIIndexFormat();
    const uint32_t indexSize = (format == DXGI_FORMAT_R32_UINT) ? 4u : 2u;

    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation = dx12Buffer->GetGPUAddress() + (uint64_t)indexOffset * indexSize;
    view.SizeInBytes    = dx12Buffer->GetSizeInBytes() - (uint64_t)indexOffset * indexSize;
    view.Format         = format;
    mCommandList->IASetIndexBuffer(&view);
}

void DX12RenderEncoder::ApplyTopology(PrimitiveMode mode)
{
    if (!mCommandList)
    {
        return;
    }
    mCommandList->IASetPrimitiveTopology(DX12Util::ConvertPrimitiveMode(mode));
}

// ============================================================================
// 绘制
// ============================================================================

void DX12RenderEncoder::DrawPrimitives(PrimitiveMode mode, int offset, int size)
{
    if (!mCommandList || !mGraphicsPipeline)
    {
        return;
    }
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->DrawInstanced((UINT)size, 1, (UINT)offset, 0);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawInstancePrimitives(PrimitiveMode mode, int offset, int size,
                                               uint32_t firstInstance, uint32_t instanceCount)
{
    if (!mCommandList || !mGraphicsPipeline)
    {
        return;
    }
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->DrawInstanced((UINT)size, instanceCount, (UINT)offset, firstInstance);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer,
                                              int offset, int baseVertex)
{
    if (!mCommandList || !mGraphicsPipeline)
    {
        return;
    }
    BindIndexBuffer(buffer, 0);
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->DrawIndexedInstanced((UINT)size, 1, (UINT)offset, baseVertex, 0);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawIndexedInstancePrimitives(PrimitiveMode mode, int size,
                                                      IndexBufferPtr buffer, int offset,
                                                      uint32_t firstInstance, uint32_t instanceCount)
{
    if (!mCommandList || !mGraphicsPipeline)
    {
        return;
    }
    BindIndexBuffer(buffer, 0);
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->DrawIndexedInstanced((UINT)size, instanceCount, (UINT)offset, 0, firstInstance);
    mCommandBuffer->MarkBlockUsedByDraw();
}

namespace
{
ID3D12CommandSignature* GetIndirectSignature(
    DX12Context* context,
    D3D12_INDIRECT_ARGUMENT_TYPE argumentType,
    uint32_t stride)
{
    if (!context || !context->device || stride == 0)
    {
        return nullptr;
    }

    auto* cache = &context->drawIndirectSignatures;
    if (argumentType == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED)
    {
        cache = &context->drawIndexedIndirectSignatures;
    }
    else if (argumentType == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH)
    {
        cache = &context->dispatchMeshIndirectSignatures;
    }

    std::lock_guard<std::mutex> lock(context->indirectSignatureMutex);
    auto found = cache->find(stride);
    if (found != cache->end())
    {
        return found->second.Get();
    }

    D3D12_INDIRECT_ARGUMENT_DESC argument = {};
    argument.Type = argumentType;
    D3D12_COMMAND_SIGNATURE_DESC desc = {};
    desc.ByteStride = stride;
    desc.NumArgumentDescs = 1;
    desc.pArgumentDescs = &argument;

    ComPtr<ID3D12CommandSignature> signature;
    const HRESULT hr = context->device->CreateCommandSignature(
        &desc, nullptr, IID_PPV_ARGS(&signature));
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] CreateCommandSignature failed: type=%u stride=%u hr=0x%08X",
                  (uint32_t)argumentType, stride, (uint32_t)hr);
        return nullptr;
    }

    ID3D12CommandSignature* result = signature.Get();
    cache->emplace(stride, std::move(signature));
    return result;
}

bool ValidateIndirectBuffer(const DX12RCBufferPtr& buffer, uint32_t offset,
                            uint32_t count, uint32_t stride, uint32_t minimumStride,
                            const char* operation)
{
    if (!buffer || !buffer->GetResource() || count == 0 || stride < minimumStride)
    {
        LOG_ERROR("[DX12] %s: invalid buffer/count/stride", operation);
        return false;
    }
    const uint64_t required = (uint64_t)offset + (uint64_t)(count - 1) * stride + minimumStride;
    if (required > buffer->GetSizeInBytes())
    {
        LOG_ERROR("[DX12] %s: indirect arguments exceed buffer (%llu > %u)",
                  operation, (unsigned long long)required, buffer->GetSizeInBytes());
        return false;
    }
    return true;
}
} // namespace

void DX12RenderEncoder::DrawPrimitivesIndirect(PrimitiveMode mode, RCBufferPtr buffer, uint32_t offset,
                                               uint32_t drawCount, uint32_t stride)
{
    auto args = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!mCommandList || !mGraphicsPipeline ||
        !ValidateIndirectBuffer(args, offset, drawCount, stride,
                                sizeof(D3D12_DRAW_ARGUMENTS), "DrawPrimitivesIndirect"))
    {
        return;
    }
    ID3D12CommandSignature* signature = GetIndirectSignature(
        mContext, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, stride);
    if (!signature)
    {
        return;
    }
    mCommandBuffer->ResourceBarrier(buffer, ResourceAccessType::IndirectCommandRead);
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->ExecuteIndirect(signature, drawCount, args->GetResource(), offset, nullptr, 0);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawIndexedPrimitivesIndirect(PrimitiveMode mode, IndexBufferPtr indexBuffer,
                                                      int indexBufferOffset, RCBufferPtr indirectBuffer,
                                                      uint32_t indirectBufferOffset,
                                                      uint32_t drawCount, uint32_t stride)
{
    auto args = std::dynamic_pointer_cast<DX12RCBuffer>(indirectBuffer);
    if (!mCommandList || !mGraphicsPipeline ||
        !ValidateIndirectBuffer(args, indirectBufferOffset, drawCount, stride,
                                sizeof(D3D12_DRAW_INDEXED_ARGUMENTS),
                                "DrawIndexedPrimitivesIndirect"))
    {
        return;
    }
    ID3D12CommandSignature* signature = GetIndirectSignature(
        mContext, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, stride);
    if (!signature)
    {
        return;
    }
    mCommandBuffer->ResourceBarrier(indirectBuffer, ResourceAccessType::IndirectCommandRead);
    BindIndexBuffer(indexBuffer, indexBufferOffset);
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->ExecuteIndirect(signature, drawCount, args->GetResource(),
                                  indirectBufferOffset, nullptr, 0);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawIndexedPrimitivesIndirectCount(PrimitiveMode mode, IndexBufferPtr indexBuffer,
                                                           int indexBufferOffset, RCBufferPtr indirectBuffer,
                                                           uint32_t indirectBufferOffset,
                                                           RCBufferPtr countBuffer,
                                                           uint32_t countBufferOffset,
                                                           uint32_t maxDrawCount, uint32_t stride)
{
    auto args = std::dynamic_pointer_cast<DX12RCBuffer>(indirectBuffer);
    auto count = std::dynamic_pointer_cast<DX12RCBuffer>(countBuffer);
    if (!mCommandList || !mGraphicsPipeline || !count || !count->GetResource() ||
        (uint64_t)countBufferOffset + sizeof(uint32_t) > count->GetSizeInBytes() ||
        !ValidateIndirectBuffer(args, indirectBufferOffset, maxDrawCount, stride,
                                sizeof(D3D12_DRAW_INDEXED_ARGUMENTS),
                                "DrawIndexedPrimitivesIndirectCount"))
    {
        return;
    }
    ID3D12CommandSignature* signature = GetIndirectSignature(
        mContext, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, stride);
    if (!signature)
    {
        return;
    }
    mCommandBuffer->ResourceBarrier(indirectBuffer, ResourceAccessType::IndirectCommandRead);
    mCommandBuffer->ResourceBarrier(countBuffer, ResourceAccessType::IndirectCommandRead);
    BindIndexBuffer(indexBuffer, indexBufferOffset);
    ApplyTopology(mode);
    FlushDescriptorTables();
    mCommandList->ExecuteIndirect(signature, maxDrawCount, args->GetResource(),
                                  indirectBufferOffset, count->GetResource(), countBufferOffset);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    if (!mCommandList || !mGraphicsPipeline || !mGraphicsPipeline->IsMeshPipeline())
    {
        return;
    }

    // DispatchMesh 在 ID3D12GraphicsCommandList6 上（需要 Windows 10 2004+）
    ComPtr<ID3D12GraphicsCommandList6> commandList6;
    if (FAILED(mCommandList->QueryInterface(IID_PPV_ARGS(&commandList6))) || !commandList6)
    {
        LOG_ERROR("[DX12] ID3D12GraphicsCommandList6 不可用，无法执行 DispatchMesh "
                  "（需要 Windows 10 2004 以上）");
        return;
    }

    FlushDescriptorTables();
    commandList6->DispatchMesh(groupCountX, groupCountY, groupCountZ);
    mCommandBuffer->MarkBlockUsedByDraw();
}

void DX12RenderEncoder::DrawMeshTasksIndirect(RCBufferPtr buffer, uint32_t offset,
                                              uint32_t drawCount, uint32_t stride)
{
    auto args = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!mCommandList || !mGraphicsPipeline || !mGraphicsPipeline->IsMeshPipeline() ||
        !mContext || !mContext->isMeshShaderSupported ||
        !ValidateIndirectBuffer(args, offset, drawCount, stride,
                                sizeof(D3D12_DISPATCH_MESH_ARGUMENTS),
                                "DrawMeshTasksIndirect"))
    {
        return;
    }
    ID3D12CommandSignature* signature = GetIndirectSignature(
        mContext, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, stride);
    if (!signature)
    {
        return;
    }
    mCommandBuffer->ResourceBarrier(buffer, ResourceAccessType::IndirectCommandRead);
    FlushDescriptorTables();
    mCommandList->ExecuteIndirect(signature, drawCount, args->GetResource(), offset, nullptr, 0);
    mCommandBuffer->MarkBlockUsedByDraw();
}

NAMESPACE_RENDERCORE_END
