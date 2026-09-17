//
//  DX12CommandQueue.h
//  rendercore
//
//  D3D12 命令队列封装。
//
//  D3D12 的 DIRECT 队列本身同时支持图形、计算与拷贝，因此这里不按
//  Vulkan 的队列族划分职责，而是直接映射到 context 里的三条队列
//  （Graphics=DIRECT / Compute=COMPUTE / Copy=COPY）。
//

#ifndef GNX_ENGINE_DX12_COMMAND_QUEUE_INCLUDE_JHGSD
#define GNX_ENGINE_DX12_COMMAND_QUEUE_INCLUDE_JHGSD

#include "DX12RenderDefine.h"
#include "DX12Context.h"
#include "CommandQueue.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12RenderDevice;

class DX12CommandQueue : public CommandQueue
{
public:
    DX12CommandQueue(DX12RenderDevice* renderDevice, const DX12ContextPtr& context,
                     QueueType type, ID3D12CommandQueue* queue,
                     uint32_t queueIndex, uint32_t familyIndex);

    ~DX12CommandQueue() override = default;

    QueueType GetType() const override { return mType; }
    QueuePriority GetPriority() const override { return QueuePriority::Normal; }
    uint32_t GetQueueIndex() const override { return mQueueIndex; }
    std::string GetDescription() const override;

    CommandBufferPtr CreateCommandBuffer() override;

    ID3D12CommandQueue* GetD3D12Queue() const { return mQueue.Get(); }
    uint32_t GetFamilyIndex() const { return mFamilyIndex; }

private:
    DX12RenderDevice* mRenderDevice = nullptr;
    DX12ContextPtr mContext;
    ComPtr<ID3D12CommandQueue> mQueue;
    QueueType mType = QueueType::Graphics;
    uint32_t mQueueIndex = 0;
    uint32_t mFamilyIndex = 0;
};

using DX12CommandQueuePtr = std::shared_ptr<DX12CommandQueue>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_COMMAND_QUEUE_INCLUDE_JHGSD */
