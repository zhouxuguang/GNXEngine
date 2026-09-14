//
//  DX12DescriptorPool.cpp
//  rendercore
//

#include "DX12DescriptorPool.h"

NAMESPACE_RENDERCORE_BEGIN

DX12DescriptorPool::~DX12DescriptorPool()
{
    Destroy();
}

bool DX12DescriptorPool::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                              uint32_t capacity, bool shaderVisible, const char* debugName)
{
    if (device == nullptr || capacity == 0)
    {
        LOG_ERROR("[DX12] DX12DescriptorPool::Init invalid args (device=%p, capacity=%u, name=%s)",
                  (void*)device, capacity, debugName ? debugName : "?");
        return false;
    }

    mType = type;
    mCapacity = capacity;
    mShaderVisible = shaderVisible;
    mUsedCount = 0;
    mPeakUsedCount = 0;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type           = type;
    heapDesc.NumDescriptors = capacity;
    heapDesc.Flags          = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                            : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask       = 0;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap));
    if (FAILED(hr))
    {
        LOG_ERROR("[DX12] CreateDescriptorHeap failed for '%s' (type=%d, count=%u): %s",
                  debugName ? debugName : "?", (int)type, capacity, DX12HResultToString(hr));
        return false;
    }

    mCpuStart = mHeap->GetCPUDescriptorHandleForHeapStart();
    if (shaderVisible)
    {
        mGpuStart = mHeap->GetGPUDescriptorHandleForHeapStart();
    }

    mIncrementSize = device->GetDescriptorHandleIncrementSize(type);

    if (debugName)
    {
        mHeap->SetName(std::wstring(debugName, debugName + strlen(debugName)).c_str());
    }

    return true;
}

void DX12DescriptorPool::Destroy()
{
    mHeap.Reset();
    mCpuStart = {};
    mGpuStart = {};
    mCapacity = 0;
    mIncrementSize = 0;
    mUsedCount = 0;
    mPeakUsedCount = 0;
    mOverflowCount = 0;
    mOverflowLogged = false;
}

bool DX12DescriptorPool::Allocate(uint32_t count, uint32_t& outStartIndex)
{
    if (count == 0)
    {
        outStartIndex = 0;
        return true;
    }

    if (mUsedCount + count > mCapacity)
    {
        ++mOverflowCount;
        if (!mOverflowLogged)
        {
            mOverflowLogged = true;
            LOG_ERROR("[DX12] Descriptor pool exhausted: type=%d capacity=%u used=%u requested=%u. "
                      "Descriptors will be recycled from the heap start; rendering this frame may be incorrect. "
                      "Consider increasing the pool capacity in DX12RenderDefine.h.",
                      (int)mType, mCapacity, mUsedCount, count);
        }

        // 回绕复用：至少保证不崩溃。正常情况下不应触发（容量已按实测留足余量）。
        mUsedCount = 0;
        if (count > mCapacity)
        {
            outStartIndex = 0;
            return false;
        }
    }

    outStartIndex = mUsedCount;
    mUsedCount += count;
    if (mUsedCount > mPeakUsedCount)
    {
        mPeakUsedCount = mUsedCount;
    }
    return true;
}

void DX12DescriptorPool::Reset()
{
    mUsedCount = 0;
    mOverflowLogged = false;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorPool::GetCpuHandle(uint32_t index) const
{
    return DX12OffsetCpuHandle(mCpuStart, index, mIncrementSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorPool::GetGpuHandle(uint32_t index) const
{
    if (!mShaderVisible)
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE{};
    }
    return DX12OffsetGpuHandle(mGpuStart, index, mIncrementSize);
}

NAMESPACE_RENDERCORE_END
