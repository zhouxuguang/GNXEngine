//
//  DX12CommandBuffer.cpp
//  rendercore
//

#include "DX12CommandBuffer.h"
#include "DX12RenderDevice.h"   // 需要完整定义以调用 SignalFrameFence / AdvanceFrameIndex
#include "DX12RenderEncoder.h"
#include "DX12ComputeEncoder.h"
#include "DX12BlitEncoder.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12Util.h"
#include "DX12Helpers.h"

#include <algorithm>

NAMESPACE_RENDERCORE_BEGIN

DX12CommandBuffer::DX12CommandBuffer(ID3D12GraphicsCommandList* commandList,
                                     ID3D12CommandAllocator* commandAllocator,
                                     const std::shared_ptr<DX12CommandBufferInfo>& info)
    : mCommandList(commandList)
    , mCommandAllocator(commandAllocator)
    , mInfo(info)
    , mContext(info ? info->context : nullptr)
    , mSwapChain(info ? info->swapChain : nullptr)
    , mRenderDevice(info ? info->renderDevice : nullptr)
    , mFrameIndex(info ? info->frameIndex : 0)
    , mBackBufferIndex(info ? info->backBufferIndex : 0)
    , mIsOffscreen(info ? info->isOffscreen : false)
    , mIsCompute(info ? info->isCompute : false)
{
    if (commandList == nullptr || mContext == nullptr)
    {
        LOG_ERROR("[DX12] DX12CommandBuffer created with invalid arguments");
        return;
    }

    EnsureDescriptorPools();
}

DX12CommandBuffer::~DX12CommandBuffer()
{
}

bool DX12CommandBuffer::EnsureDescriptorPools()
{
    if (mCbvSrvUavPool && mSamplerPool && mBindCbvSrvUavPool && mBindSamplerPool)
    {
        return true;
    }

    // ---- 写入堆（非 shader-visible）----
    mCbvSrvUavPool = std::make_unique<DX12DescriptorPool>();
    if (!mCbvSrvUavPool->Init(mContext->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                              GNX_DX12_FRAME_CBV_SRV_UAV_CAPACITY, /*shaderVisible*/ false,
                              "DX12 CBV/SRV/UAV Ring (write)"))
    {
        return false;
    }

    // 采样器堆受硬件上限 2048 约束
    mSamplerPool = std::make_unique<DX12DescriptorPool>();
    if (!mSamplerPool->Init(mContext->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                            GNX_DX12_FRAME_SAMPLER_CAPACITY, /*shaderVisible*/ false,
                            "DX12 Sampler Ring (write)"))
    {
        return false;
    }

    // ---- 绑定堆（shader-visible）----
    mBindCbvSrvUavPool = std::make_unique<DX12DescriptorPool>();
    if (!mBindCbvSrvUavPool->Init(mContext->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                  GNX_DX12_FRAME_CBV_SRV_UAV_CAPACITY, /*shaderVisible*/ true,
                                  "DX12 CBV/SRV/UAV Ring (bind)"))
    {
        return false;
    }

    mBindSamplerPool = std::make_unique<DX12DescriptorPool>();
    if (!mBindSamplerPool->Init(mContext->device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                GNX_DX12_FRAME_SAMPLER_CAPACITY, /*shaderVisible*/ true,
                                "DX12 Sampler Ring (bind)"))
    {
        return false;
    }

    return true;
}

bool DX12CommandBuffer::AllocateBlockDescriptors(DescriptorBlock& block)
{
    // CBV / SRV / UAV 共用 CBV_SRV_UAV 堆，Sampler 用独立堆。
    // 两级环按完全相同的顺序与宽度分配，因此下标天然一致；
    // 一旦不一致说明分配序列被打乱（会导致寄存器错位），必须立即失败而不是继续跑。
    //
    // 每个类别的分配量是 width × groupCount：Mesh 管线需要 3 组（AS/MS/PS）连续段。
    const uint32_t cbvTotal = block.Total((size_t)DX12DescriptorClass::CBV);
    const uint32_t srvTotal = block.Total((size_t)DX12DescriptorClass::SRV);
    const uint32_t uavTotal = block.Total((size_t)DX12DescriptorClass::UAV);
    const uint32_t smpTotal = block.Total((size_t)DX12DescriptorClass::Sampler);

    if (!mCbvSrvUavPool->Allocate(cbvTotal, block.base[(size_t)DX12DescriptorClass::CBV]) ||
        !mCbvSrvUavPool->Allocate(srvTotal, block.base[(size_t)DX12DescriptorClass::SRV]) ||
        !mCbvSrvUavPool->Allocate(uavTotal, block.base[(size_t)DX12DescriptorClass::UAV]) ||
        !mSamplerPool->Allocate(smpTotal, block.base[(size_t)DX12DescriptorClass::Sampler]))
    {
        return false;
    }

    // 绑定堆镜像同样的布局（下标必须与写入堆完全一致）
    uint32_t bindCbv = 0;
    uint32_t bindSrv = 0;
    uint32_t bindUav = 0;
    uint32_t bindSmp = 0;
    if (!mBindCbvSrvUavPool->Allocate(cbvTotal, bindCbv) ||
        !mBindCbvSrvUavPool->Allocate(srvTotal, bindSrv) ||
        !mBindCbvSrvUavPool->Allocate(uavTotal, bindUav) ||
        !mBindSamplerPool->Allocate(smpTotal, bindSmp))
    {
        return false;
    }

    if (bindCbv != block.base[(size_t)DX12DescriptorClass::CBV] ||
        bindSrv != block.base[(size_t)DX12DescriptorClass::SRV] ||
        bindUav != block.base[(size_t)DX12DescriptorClass::UAV] ||
        bindSmp != block.base[(size_t)DX12DescriptorClass::Sampler])
    {
        LOG_ERROR("[DX12] 描述符写入堆与绑定堆下标不一致（写入 %u/%u/%u/%u，绑定 %u/%u/%u/%u）："
                  "两级环的分配序列被破坏，寄存器编号会错位",
                  block.base[0], block.base[1], block.base[2], block.base[3],
                  bindCbv, bindSrv, bindUav, bindSmp);
        return false;
    }

    return true;
}

void DX12CommandBuffer::SyncBlockToBindHeaps()
{
    if (!mBlockValid || !mContext || !mContext->device)
    {
        return;
    }

    // 整块拷贝：绑定堆必须完整镜像写入堆，否则未重新绑定的资源会指向旧数据。
    // 源是普通堆、目标是 shader-visible 堆，这是 D3D12 允许的方向。
    // 拷贝范围按 Total()（含全部 stage 组），否则 Mesh 管线的 MS/PS 组会读到旧数据。
    for (uint32_t cls = 0; cls < (uint32_t)DX12DescriptorClass::Count; ++cls)
    {
        const bool isSampler = (cls == (uint32_t)DX12DescriptorClass::Sampler);
        DX12DescriptorPool* src = isSampler ? mSamplerPool.get() : mCbvSrvUavPool.get();
        DX12DescriptorPool* dst = isSampler ? mBindSamplerPool.get() : mBindCbvSrvUavPool.get();
        if (src == nullptr || dst == nullptr || !src->IsValid() || !dst->IsValid())
        {
            continue;
        }

        const uint32_t total = mBlock.Total(cls);
        if (total == 0)
        {
            continue;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = dst->GetCpuHandle(mBlock.base[cls]);
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = src->GetCpuHandle(mBlock.base[cls]);
        UINT rangeSizes[1] = { total };
        mContext->device->CopyDescriptors(
            1, &dstHandle, rangeSizes, 1, &srcHandle, rangeSizes,
            isSampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
                      : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

namespace
{
/// 管线声明的表宽按根签名上限截断后，即为该管线实际需要的最小宽度
inline uint32_t ClampTableWidth(uint32_t declaredWidth, uint32_t limit)
{
    return (std::min)(declaredWidth, limit);
}
} // namespace

bool DX12CommandBuffer::BlockCoversPipeline(DX12GraphicsPipeline* pipeline) const
{
    if (!mBlockValid || pipeline == nullptr)
    {
        return false;
    }

    // stage 组数必须一致：块的物理布局是按组数分配的（每组 width 个连续描述符），
    // 组数不同意味着同一类别内各组的起始下标完全不同，必须重新分配块。
    if (mBlock.groupCount != pipeline->GetStageGroupCount())
    {
        return false;
    }

    const uint32_t required[(size_t)DX12DescriptorClass::Count] = {
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::CBV), GNX_DX12_CBV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::SRV), GNX_DX12_SRV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::UAV), GNX_DX12_UAV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::Sampler),
                        GNX_DX12_SAMPLER_TABLE_WIDTH),
    };

    for (uint32_t cls = 0; cls < (uint32_t)DX12DescriptorClass::Count; ++cls)
    {
        if (mBlock.width[cls] < required[cls])
        {
            return false;
        }
    }
    return true;
}

bool DX12CommandBuffer::BlockCoversPipeline(DX12ComputePipeline* pipeline) const
{
    if (!mBlockValid || pipeline == nullptr)
    {
        return false;
    }

    const uint32_t required[(size_t)DX12DescriptorClass::Count] = {
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::CBV), GNX_DX12_CBV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::SRV), GNX_DX12_SRV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::UAV), GNX_DX12_UAV_TABLE_WIDTH),
        ClampTableWidth(pipeline->GetTableWidth(DX12DescriptorClass::Sampler),
                        GNX_DX12_SAMPLER_TABLE_WIDTH),
    };

    for (uint32_t cls = 0; cls < (uint32_t)DX12DescriptorClass::Count; ++cls)
    {
        if (mBlock.width[cls] < required[cls])
        {
            return false;
        }
    }
    return true;
}

void DX12CommandBuffer::BindDescriptorHeaps()
{
    if (!mCommandList || !mCbvSrvUavPool || !mSamplerPool)
    {
        return;
    }

    // 必须绑定 shader-visible 的绑定堆；写入堆只用于 CPU 侧写入与继承拷贝
    ID3D12DescriptorHeap* heaps[] = { mBindCbvSrvUavPool->GetHeap(),
                                      mBindSamplerPool->GetHeap() };
    mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

bool DX12CommandBuffer::BeginRecording()
{
    if (!mCommandList || !mCommandAllocator || !mContext)
    {
        return false;
    }

    // 若上一次录制没有正常结束（宿主可能创建了命令缓冲区但未提交/未结束录制），
    // 命令列表仍处于 Recording 状态，此时 Reset 分配器会直接失败：
    //   "The command allocator cannot be reset because a command list is currently
    //    being recorded with the allocator"（id=543）
    // 这里先关闭残留的列表，保证分配器可以安全 Reset。
    if (mState == State::Recording)
    {
        mCommandList->Close();
        mState = State::Initial;
    }

    // 命令分配器在 GPU 使用期间不能 Reset；调用方（RenderDevice）已确保帧槽位空闲
    const HRESULT hrAllocator = mCommandAllocator->Reset();
    if (FAILED(hrAllocator))
    {
        LOG_ERROR("[DX12] ID3D12CommandAllocator::Reset failed: %s",
                  DX12HResultToString(hrAllocator));
        return false;
    }

    const HRESULT hrList = mCommandList->Reset(mCommandAllocator.Get(), nullptr);
    if (FAILED(hrList))
    {
        LOG_ERROR("[DX12] ID3D12GraphicsCommandList::Reset failed: %s",
                  DX12HResultToString(hrList));
        return false;
    }

    // 重置描述符环形堆：此时 GPU 已确认不再读取上一帧的描述符
    if (mCbvSrvUavPool)     mCbvSrvUavPool->Reset();
    if (mSamplerPool)       mSamplerPool->Reset();
    if (mBindCbvSrvUavPool) mBindCbvSrvUavPool->Reset();
    if (mBindSamplerPool)   mBindSamplerPool->Reset();

    mBlockValid = false;
    mBlockUsedByDraw = false;
    mBlock = {};
    mCurrentRootSignature = nullptr;
    mCurrentGraphicsPipeline = nullptr;
    mCurrentComputePipeline = nullptr;
    mCurrentIsMeshPipeline = false;
    mDebugGroupDepth = 0;

    // 绑定本命令缓冲区专属的描述符堆（整个录制期间不变）
    BindDescriptorHeaps();

    mState = State::Recording;
    return true;
}

void DX12CommandBuffer::EndRecording()
{
    if (mState != State::Recording || !mCommandList)
    {
        return;
    }

    // 关闭未配对的调试组，避免调试层报错
    while (mDebugGroupDepth > 0)
    {
        mCommandList->EndEvent();
        --mDebugGroupDepth;
    }

    const HRESULT hr = mCommandList->Close();
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] ID3D12GraphicsCommandList::Close failed: %s",
                  DX12HResultToString(hr));
        return;
    }

    mState = State::Ended;
}

// ============================================================================
// 描述符环形分配
// ============================================================================

void DX12CommandBuffer::InheritDescriptors(const DescriptorBlock& oldBlock,
                                           const DescriptorBlock& newBlock)
{
    // 把旧块的描述符整体拷贝到新块：这是对齐 Vulkan push descriptor
    // “绑定持续有效直到被覆盖”语义的关键。旧块在环形堆里仍然有效，
    // CopyDescriptors 是纯 CPU 侧操作，代价可忽略。
    //
    // 组数不同时只继承第 0 组：新旧块的组布局不同，第 1/2 组在旧块里的
    // 对应位置属于别的数据，拷过去只会把无关描述符混进 stage 组。
    const uint32_t inheritGroups = (oldBlock.groupCount == newBlock.groupCount)
                                       ? newBlock.groupCount
                                       : 1;

    for (uint32_t cls = 0; cls < (uint32_t)DX12DescriptorClass::Count; ++cls)
    {
        const uint32_t count = (std::min)(oldBlock.width[cls], newBlock.width[cls]) * inheritGroups;
        if (count == 0)
        {
            continue;
        }

        const bool isSampler = (cls == (uint32_t)DX12DescriptorClass::Sampler);
        DX12DescriptorPool& pool = isSampler ? *mSamplerPool : *mCbvSrvUavPool;

        D3D12_CPU_DESCRIPTOR_HANDLE dest = pool.GetCpuHandle(newBlock.base[cls]);
        D3D12_CPU_DESCRIPTOR_HANDLE src  = pool.GetCpuHandle(oldBlock.base[cls]);

        UINT rangeSizes[1] = { count };
        mContext->device->CopyDescriptors(
            1, &dest, rangeSizes, 1, &src, rangeSizes,
            isSampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
                      : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

bool DX12CommandBuffer::AcquireDescriptorBlock(DX12GraphicsPipeline* graphicsPipeline)
{
    // 当前块尚未被任何 Draw 使用：直接复用，继续往里写
    // 当前块尚未被任何 Draw 使用，且表宽已覆盖该管线：直接复用，继续往里写。
    // 覆盖性校验不可省略：块是按上一条管线的表宽分配的，宽度不足时复用会导致
    // 绑定越界写（GetCpuHandle 返回空句柄 → CreateUnorderedAccessView 报 id=646）。
    if (!mBlockUsedByDraw && BlockCoversPipeline(graphicsPipeline))
    {
        return true;
    }
    if (!graphicsPipeline || !EnsureDescriptorPools())
    {
        return false;
    }

    DescriptorBlock block;

    // 表宽度来自合并后的反射（= 各类最大寄存器号 + 1），并限制在根签名声明的上限内
    block.width[(size_t)DX12DescriptorClass::CBV] =
        (std::min)(graphicsPipeline->GetTableWidth(DX12DescriptorClass::CBV),
                   GNX_DX12_CBV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::SRV] =
        (std::min)(graphicsPipeline->GetTableWidth(DX12DescriptorClass::SRV),
                   GNX_DX12_SRV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::UAV] =
        (std::min)(graphicsPipeline->GetTableWidth(DX12DescriptorClass::UAV),
                   GNX_DX12_UAV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::Sampler] =
        (std::min)(graphicsPipeline->GetTableWidth(DX12DescriptorClass::Sampler),
                   GNX_DX12_SAMPLER_TABLE_WIDTH);

    // Mesh 管线需要 AS / MS / PS 三组表（各 stage 的寄存器独立编号）
    block.groupCount = graphicsPipeline->GetStageGroupCount();

    // CBV / SRV / UAV 共用一个 CBV_SRV_UAV 堆，各自分配连续段
    // 写入堆 + 绑定堆同步分配（见 AllocateBlockDescriptors）
    if (!AllocateBlockDescriptors(block))
    {
        return false;
    }

    // 旧块 → 新块：继承描述符，保持“未重新绑定的资源继续生效”
    if (mBlockValid)
    {
        InheritDescriptors(mBlock, block);
    }

    mBlock = block;
    mBlockValid = true;
    mBlockUsedByDraw = false;
    return true;
}

bool DX12CommandBuffer::AcquireComputeDescriptorBlock(DX12ComputePipeline* computePipeline)
{
    // 同上：复用前必须校验表宽覆盖性
    if (!mBlockUsedByDraw && BlockCoversPipeline(computePipeline))
    {
        return true;
    }
    if (!computePipeline || !EnsureDescriptorPools())
    {
        return false;
    }

    DescriptorBlock block;
    block.width[(size_t)DX12DescriptorClass::CBV] =
        (std::min)(computePipeline->GetTableWidth(DX12DescriptorClass::CBV),
                   GNX_DX12_CBV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::SRV] =
        (std::min)(computePipeline->GetTableWidth(DX12DescriptorClass::SRV),
                   GNX_DX12_SRV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::UAV] =
        (std::min)(computePipeline->GetTableWidth(DX12DescriptorClass::UAV),
                   GNX_DX12_UAV_TABLE_WIDTH);
    block.width[(size_t)DX12DescriptorClass::Sampler] =
        (std::min)(computePipeline->GetTableWidth(DX12DescriptorClass::Sampler),
                   GNX_DX12_SAMPLER_TABLE_WIDTH);

    // 写入堆 + 绑定堆同步分配（见 AllocateBlockDescriptors）
    if (!AllocateBlockDescriptors(block))
    {
        return false;
    }

    if (mBlockValid)
    {
        InheritDescriptors(mBlock, block);
    }

    mBlock = block;
    mBlockValid = true;
    mBlockUsedByDraw = false;
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12CommandBuffer::GetCpuHandle(DX12DescriptorClass cls,
                                                            uint32_t registerIndex,
                                                            uint32_t stageGroup) const
{
    const uint32_t classIndex = (uint32_t)cls;
    const bool classValid = (classIndex < (uint32_t)DX12DescriptorClass::Count);
    const bool groupValid = (stageGroup < mBlock.groupCount);

    if (!mBlockValid || !classValid || !groupValid ||
        registerIndex >= mBlock.width[classIndex])
    {
        LOG_ERROR("[DX12] GetCpuHandle: invalid access (class=%u, reg=%u, group=%u, width=%u)",
                  classIndex, registerIndex, stageGroup,
                  classValid ? mBlock.width[classIndex] : 0);
        return D3D12_CPU_DESCRIPTOR_HANDLE{};
    }

    // 块内布局：[group0 的 width 个槽位][group1 的 width 个槽位][group2 …]
    const uint32_t index = mBlock.base[classIndex] + stageGroup * mBlock.width[classIndex]
                           + registerIndex;

    if (cls == DX12DescriptorClass::Sampler)
    {
        return mSamplerPool->GetCpuHandle(index);
    }
    return mCbvSrvUavPool->GetCpuHandle(index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12CommandBuffer::GetGpuHandle(DX12DescriptorClass cls,
                                                            uint32_t stageGroup) const
{
    const uint32_t classIndex = (uint32_t)cls;
    if (!mBlockValid || classIndex >= (uint32_t)DX12DescriptorClass::Count ||
        stageGroup >= mBlock.groupCount)
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }

    const uint32_t index = mBlock.base[classIndex] + stageGroup * mBlock.width[classIndex];

    if (cls == DX12DescriptorClass::Sampler)
    {
        return mBindSamplerPool->GetGpuHandle(index);
    }
    return mBindCbvSrvUavPool->GetGpuHandle(index);
}

uint32_t DX12CommandBuffer::GetBlockWidth(DX12DescriptorClass cls) const
{
    const uint32_t classIndex = (uint32_t)cls;
    return (classIndex < (uint32_t)DX12DescriptorClass::Count) ? mBlock.width[classIndex] : 0;
}

void DX12CommandBuffer::BindGraphicsDescriptorTables(bool isMeshPipeline)
{
    if (!mCommandList || !mBlockValid)
    {
        return;
    }

    // 先把当前块同步到 shader-visible 绑定堆，再设置根表
    SyncBlockToBindHeaps();

    if (mCurrentRootSignature != nullptr)
    {
        mCommandList->SetGraphicsRootSignature(mCurrentRootSignature.Get());
    }

    // 根参数按 stage 组排列：
    //   非 Mesh：0..3        = CBV/SRV/UAV/Sampler（SHADER_VISIBILITY_ALL）
    //   Mesh   ：0..3 = AS、4..7 = MS、8..11 = PS（见 DX12RootSignature.h）
    // 每组指向本组在描述符块中的起始槽位 —— 组内偏移即该 stage 自己的寄存器号。
    const uint32_t groupCount = isMeshPipeline
                                    ? mBlock.groupCount
                                    : 1u;
    for (uint32_t group = 0; group < groupCount; ++group)
    {
        for (uint32_t i = 0; i < GNX_DX12_ROOT_PARAM_COUNT; ++i)
        {
            mCommandList->SetGraphicsRootDescriptorTable(
                group * GNX_DX12_ROOT_PARAM_COUNT + i,
                GetGpuHandle((DX12DescriptorClass)i, group));
        }
    }

}

void DX12CommandBuffer::BindComputeDescriptorTables()
{
    if (!mCommandList || !mBlockValid)
    {
        return;
    }

    // 先把当前块同步到 shader-visible 绑定堆，再设置根表
    SyncBlockToBindHeaps();

    if (mCurrentRootSignature != nullptr)
    {
        mCommandList->SetComputeRootSignature(mCurrentRootSignature.Get());
    }

    for (uint32_t i = 0; i < GNX_DX12_ROOT_PARAM_COUNT; ++i)
    {
        mCommandList->SetComputeRootDescriptorTable(i, GetGpuHandle((DX12DescriptorClass)i));
    }
}

// ============================================================================
// 资源屏障
// ============================================================================

void DX12CommandBuffer::ResourceBarrier(RCTexturePtr texture, ResourceAccessType accessType)
{
    auto dx12Texture = std::dynamic_pointer_cast<DX12TextureBase>(texture);
    if (!dx12Texture || !dx12Texture->GetResource() || !mCommandList)
    {
        return;
    }

    ID3D12Resource* resource = dx12Texture->GetResource();
    const D3D12_RESOURCE_STATES newState =
        DX12Util::ConvertTextureAccessType(accessType, dx12Texture->GetDXGIFormat());
    const D3D12_RESOURCE_STATES currentState = dx12Texture->GetCurrentState();

    if (newState == currentState)
    {
        return;
    }

    // UAV → UAV 的转换必须用 UAV barrier（transition barrier 不合法）
    if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
        currentState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        const D3D12_RESOURCE_BARRIER barrier = DX12UAVBarrier(resource);
        mCommandList->ResourceBarrier(1, &barrier);
        return;
    }

    const D3D12_RESOURCE_BARRIER barrier =
        DX12TransitionBarrier(resource, currentState, newState);
    mCommandList->ResourceBarrier(1, &barrier);
    dx12Texture->SetCurrentState(newState);
}

void DX12CommandBuffer::ResourceBarrier(RCBufferPtr buffer, ResourceAccessType accessType)
{
    auto dx12Buffer = std::dynamic_pointer_cast<DX12RCBuffer>(buffer);
    if (!dx12Buffer || !dx12Buffer->GetResource() || !mCommandList)
    {
        return;
    }

    ID3D12Resource* resource = dx12Buffer->GetResource();
    const D3D12_RESOURCE_STATES newState = DX12Util::ConvertBufferAccessType(accessType);
    const D3D12_RESOURCE_STATES currentState = dx12Buffer->GetCurrentState();

    if (newState == currentState)
    {
        return;
    }

    if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
        currentState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        const D3D12_RESOURCE_BARRIER barrier = DX12UAVBarrier(resource);
        mCommandList->ResourceBarrier(1, &barrier);
        return;
    }

    const D3D12_RESOURCE_BARRIER barrier =
        DX12TransitionBarrier(resource, currentState, newState);
    mCommandList->ResourceBarrier(1, &barrier);
    dx12Buffer->SetCurrentState(newState);
}

// ============================================================================
// Encoder 创建
// ============================================================================

RenderEncoderPtr DX12CommandBuffer::CreateDefaultRenderEncoder(const ClearColor& clearColor) const
{
    if (mState != State::Recording || mIsOffscreen)
    {
        LOG_ERROR("[DX12] CreateDefaultRenderEncoder: command buffer is not recording "
                  "or is offscreen");
        return nullptr;
    }

    return std::make_shared<DX12RenderEncoder>(
        const_cast<DX12CommandBuffer*>(this)->shared_from_this(), clearColor, true);
}

RenderEncoderPtr DX12CommandBuffer::CreateRenderEncoder(const RenderPass& renderPass) const
{
    if (mState != State::Recording)
    {
        LOG_ERROR("[DX12] CreateRenderEncoder: command buffer is not recording");
        return nullptr;
    }

    return std::make_shared<DX12RenderEncoder>(
        const_cast<DX12CommandBuffer*>(this)->shared_from_this(), renderPass);
}

ComputeEncoderPtr DX12CommandBuffer::CreateComputeEncoder() const
{
    if (mState != State::Recording)
    {
        LOG_ERROR("[DX12] CreateComputeEncoder: command buffer is not recording");
        return nullptr;
    }

    return std::make_shared<DX12ComputeEncoder>(
        const_cast<DX12CommandBuffer*>(this)->shared_from_this());
}

BlitEncoderPtr DX12CommandBuffer::CreateBlitEncoder() const
{
    if (mState != State::Recording)
    {
        LOG_ERROR("[DX12] CreateBlitEncoder: command buffer is not recording");
        return nullptr;
    }

    return std::make_shared<DX12BlitEncoder>(
        const_cast<DX12CommandBuffer*>(this)->shared_from_this());
}

// ============================================================================
// 提交 / 上屏 / 等待
// ============================================================================

void DX12CommandBuffer::BeginDebugGroup(const char* name, const float color[4])
{
    (void)name;
    (void)color;
}

void DX12CommandBuffer::EndDebugGroup()
{
    if (!mCommandList || mState != State::Recording || mDebugGroupDepth == 0)
    {
        return;
    }

    mCommandList->EndEvent();
    --mDebugGroupDepth;
}

bool DX12CommandBuffer::PresentToSwapChain()
{
    if (mIsOffscreen || !mSwapChain)
    {
        return false;
    }

    EndRecording();
    if (mState != State::Ended)
    {
        return false;
    }

    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mContext->graphicsQueue->ExecuteCommandLists(1, lists);

    // 必须先 signal 本帧槽位的栅栏，再让设备推进帧索引：
    // 否则下一次用到该槽位时无法等待 GPU 完成，命令分配器会在使用中被 Reset。
    if (mRenderDevice != nullptr)
    {
        mRenderDevice->SignalFrameFence(mFrameIndex);
    }
    mContext->graphicsFence.Signal(mContext->graphicsQueue.Get());

    if (!mSwapChain->Present())
    {
        return false;
    }

    mState = State::Submitted;

    if (mRenderDevice != nullptr)
    {
        mRenderDevice->AdvanceFrameIndex();
    }
    return true;
}

void DX12CommandBuffer::PresentFrameBuffer()
{
    if (mIsOffscreen)
    {
        // 离屏命令缓冲区没有可上屏的内容，按 Vulkan 后端语义立即提交并等待
        Submit();
        WaitUntilCompleted();
        return;
    }

    PresentToSwapChain();
}

void DX12CommandBuffer::Submit()
{
    if (mState == State::Submitted)
    {
        return;
    }

    EndRecording();
    if (mState != State::Ended || !mContext)
    {
        return;
    }

    ID3D12CommandList* lists[] = { mCommandList.Get() };
    ID3D12CommandQueue* queue = mIsCompute && mContext->computeQueue
                                    ? mContext->computeQueue.Get()
                                    : mContext->graphicsQueue.Get();
    queue->ExecuteCommandLists(1, lists);

    if (mIsCompute && mContext->computeQueue)
    {
        mContext->computeFence.Signal(mContext->computeQueue.Get());
    }
    else
    {
        mContext->graphicsFence.Signal(mContext->graphicsQueue.Get());
        if (mRenderDevice != nullptr)
        {
            mRenderDevice->SignalFrameFence(mFrameIndex);
        }
    }

    mState = State::Submitted;
}

void DX12CommandBuffer::WaitUntilCompleted()
{
    if (!mContext)
    {
        return;
    }

    // 必须先提交已录制的命令，再等待完成。
    //
    // 这是引擎的既有契约：调用方（例如大气 LUT 预计算）为每个 pass 新建一个
    // 离屏命令缓冲区，录制完成后**只调用 WaitUntilCompleted()** 来表达
    // "提交并同步"。若这里只等待而不提交，命令就永远不会执行——渲染目标只保留
    // 清屏值（LUT 全零），天空整体变黑，而且不会产生任何 D3D12 校验错误。
    Submit();

    if (mIsCompute && mContext->computeQueue)
    {
        mContext->computeFence.WaitForIdle(10000);
    }
    else
    {
        mContext->graphicsFence.WaitForIdle(10000);
    }
}

NAMESPACE_RENDERCORE_END
