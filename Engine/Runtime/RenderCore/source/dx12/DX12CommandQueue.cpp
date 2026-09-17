//
//  DX12CommandQueue.cpp
//  rendercore
//

#include "DX12CommandQueue.h"
#include "DX12RenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

DX12CommandQueue::DX12CommandQueue(DX12RenderDevice* renderDevice, const DX12ContextPtr& context,
                                   QueueType type, ID3D12CommandQueue* queue,
                                   uint32_t queueIndex, uint32_t familyIndex)
    : mRenderDevice(renderDevice)
    , mContext(context)
    , mQueue(queue)
    , mType(type)
    , mQueueIndex(queueIndex)
    , mFamilyIndex(familyIndex)
{
}

std::string DX12CommandQueue::GetDescription() const
{
    return std::string(GetQueueTypeName(mType)) + " Queue (DX12 DIRECT/COMPUTE/COPY #" +
           std::to_string(mQueueIndex) + ")";
}

CommandBufferPtr DX12CommandQueue::CreateCommandBuffer()
{
    if (mRenderDevice == nullptr)
    {
        return nullptr;
    }
    return mRenderDevice->CreateCommandBuffer();
}

NAMESPACE_RENDERCORE_END
