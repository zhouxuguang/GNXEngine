//
//  DX12Helpers.h
//  rendercore
//
//  引擎特有的视图创建辅助。通用 D3D12 构造使用官方 d3dx12 helper。
//

#ifndef GNX_ENGINE_DX12_HELPERS_INCLUDE_JHGDS
#define GNX_ENGINE_DX12_HELPERS_INCLUDE_JHGDS

#include "DX12RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// 描述符句柄算术
// ============================================================================

inline D3D12_CPU_DESCRIPTOR_HANDLE DX12OffsetCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
                                                       uint32_t offset, uint32_t incrementSize)
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(handle, (INT)offset, incrementSize);
}

inline D3D12_GPU_DESCRIPTOR_HANDLE DX12OffsetGpuHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle,
                                                       uint32_t offset, uint32_t incrementSize)
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, (INT)offset, incrementSize);
}

// ============================================================================
// 常用描述的构造
// ============================================================================

inline D3D12_HEAP_PROPERTIES DX12HeapProperties(D3D12_HEAP_TYPE type)
{
    return CD3DX12_HEAP_PROPERTIES(type);
}

inline D3D12_RESOURCE_DESC DX12BufferResourceDesc(uint64_t sizeInBytes, D3D12_RESOURCE_FLAGS flags)
{
    return CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, flags);
}

inline D3D12_RESOURCE_DESC DX12TextureResourceDesc(
    D3D12_RESOURCE_DIMENSION dimension,
    uint64_t width, uint32_t height, uint32_t depthOrArraySize, uint32_t mipLevels,
    DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, uint32_t sampleCount = 1)
{
    return CD3DX12_RESOURCE_DESC(dimension, 0, width,
                                 dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? 1u : height,
                                 (UINT16)depthOrArraySize, (UINT16)mipLevels, format, sampleCount, 0,
                                 D3D12_TEXTURE_LAYOUT_UNKNOWN, flags);
}

inline D3D12_RANGE DX12ReadRange(uint64_t begin, uint64_t end)
{
    return CD3DX12_RANGE((SIZE_T)begin, (SIZE_T)end);
}

inline D3D12_RANGE DX12NoWriteRange()
{
    // Map 时表示“CPU 不会写入”，可避免不必要的缓存失效
    return CD3DX12_RANGE(0, 0);
}

inline D3D12_RESOURCE_BARRIER DX12TransitionBarrier(ID3D12Resource* resource,
                                                    D3D12_RESOURCE_STATES stateBefore,
                                                    D3D12_RESOURCE_STATES stateAfter,
                                                    uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                    D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
{
    return CD3DX12_RESOURCE_BARRIER::Transition(resource, stateBefore, stateAfter, subresource, flags);
}

inline D3D12_RESOURCE_BARRIER DX12UAVBarrier(ID3D12Resource* resource = nullptr)
{
    return CD3DX12_RESOURCE_BARRIER::UAV(resource);
}

inline D3D12_SHADER_RESOURCE_VIEW_DESC DX12TextureSRVDesc(DXGI_FORMAT format,
                                                          D3D12_SRV_DIMENSION dimension,
                                                          uint32_t mipLevels,
                                                          uint32_t mostDetailedMip = 0,
                                                          uint32_t firstArraySlice = 0,
                                                          uint32_t arraySize = 1)
{
    switch (dimension)
    {
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            return CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(format, mipLevels, mostDetailedMip);
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            return CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2DArray(
                format, arraySize, mipLevels, firstArraySlice, mostDetailedMip);
        case D3D12_SRV_DIMENSION_TEXTURE3D:
            return CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex3D(format, mipLevels, mostDetailedMip);
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            return CD3DX12_SHADER_RESOURCE_VIEW_DESC::TexCube(format, mipLevels, mostDetailedMip);
        default:
            return {};
    }
}

inline D3D12_UNORDERED_ACCESS_VIEW_DESC DX12TextureUAVDesc(DXGI_FORMAT format,
                                                           D3D12_UAV_DIMENSION dimension,
                                                           uint32_t mipSlice = 0,
                                                           uint32_t firstArraySlice = 0,
                                                           uint32_t arraySize = 1,
                                                           uint32_t depth = 1)
{
    switch (dimension)
    {
        case D3D12_UAV_DIMENSION_TEXTURE2D:
            return CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(format, mipSlice);
        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
            return CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2DArray(
                format, arraySize, firstArraySlice, mipSlice);
        case D3D12_UAV_DIMENSION_TEXTURE3D:
            return CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex3D(format, depth, 0, mipSlice);
        default:
            return {};
    }
}

inline D3D12_RENDER_TARGET_VIEW_DESC DX12RTVDesc(DXGI_FORMAT format,
                                                 D3D12_RTV_DIMENSION dimension,
                                                 uint32_t mipSlice = 0,
                                                 uint32_t firstArraySlice = 0,
                                                 uint32_t arraySize = 1)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format        = format;
    desc.ViewDimension = dimension;

    if (dimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY)
    {
        desc.Texture2DArray.MipSlice        = mipSlice;
        desc.Texture2DArray.FirstArraySlice = firstArraySlice;
        desc.Texture2DArray.ArraySize       = arraySize;
        desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (dimension == D3D12_RTV_DIMENSION_TEXTURE2D)
    {
        desc.Texture2D.MipSlice   = mipSlice;
        desc.Texture2D.PlaneSlice = 0;
    }

    return desc;
}

inline D3D12_DEPTH_STENCIL_VIEW_DESC DX12DSVDesc(DXGI_FORMAT format,
                                                 D3D12_DSV_DIMENSION dimension,
                                                 uint32_t mipSlice = 0,
                                                 uint32_t firstArraySlice = 0,
                                                 uint32_t arraySize = 1)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
    desc.Format        = format;
    desc.ViewDimension = dimension;
    desc.Flags         = D3D12_DSV_FLAG_NONE;

    if (dimension == D3D12_DSV_DIMENSION_TEXTURE2DARRAY)
    {
        desc.Texture2DArray.MipSlice        = mipSlice;
        desc.Texture2DArray.FirstArraySlice = firstArraySlice;
        desc.Texture2DArray.ArraySize       = arraySize;
    }
    else if (dimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        desc.Texture2D.MipSlice = mipSlice;
    }

    return desc;
}

inline D3D12_SHADER_RESOURCE_VIEW_DESC DX12BufferSRVDesc(uint64_t firstElement,
                                                         uint32_t numElements,
                                                         uint32_t structureByteStride)
{
    if (structureByteStride > 0)
    {
        return CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(
            numElements, structureByteStride, firstElement);
    }
    return CD3DX12_SHADER_RESOURCE_VIEW_DESC::RawBuffer(numElements, firstElement);
}

inline D3D12_UNORDERED_ACCESS_VIEW_DESC DX12BufferUAVDesc(uint64_t firstElement,
                                                          uint32_t numElements,
                                                          uint32_t structureByteStride,
                                                          uint64_t counterOffset = 0)
{
    if (structureByteStride > 0)
    {
        return CD3DX12_UNORDERED_ACCESS_VIEW_DESC::StructuredBuffer(
            numElements, structureByteStride, firstElement, counterOffset);
    }
    return CD3DX12_UNORDERED_ACCESS_VIEW_DESC::RawBuffer(
        numElements, firstElement, counterOffset);
}

inline D3D12_CONSTANT_BUFFER_VIEW_DESC DX12CBVDesc(D3D12_GPU_VIRTUAL_ADDRESS bufferLocation,
                                                   uint32_t sizeInBytes)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
    desc.BufferLocation = bufferLocation;
    // CBV 大小必须是 256 字节向上对齐
    desc.SizeInBytes = (uint32_t)((sizeInBytes + 255u) & ~255u);
    return desc;
}

/**
 * @brief 为缓冲区创建 SRV / UAV
 *
 * Raw（字节寻址）与结构化视图的创建方式完全不同，且必须与 shader 的声明一致，
 * 否则 D3D12 会因视图类型不匹配而校验失败（表现为完全不渲染）：
 *
 *   ByteAddressBuffer / RWByteAddressBuffer
 *       → Format = R32_TYPELESS, StructureByteStride = 0, FLAG_RAW
 *       → 元素数 = 字节数 / 4
 *       → **不需要知道元素步长**
 *
 *   StructuredBuffer<T> / RWStructuredBuffer<T>
 *       → Format = UNKNOWN, StructureByteStride = sizeof(T)
 *       → 元素数 = 字节数 / 步长
 *
 * @return 成功返回 true；参数不足（缺少必需的步长等）返回 false
 */
inline bool DX12CreateBufferView(ID3D12Device* device, ID3D12Resource* resource,
                                uint64_t sizeInBytes, bool asUAV, bool isRawBuffer,
                                uint32_t structuredStride, D3D12_CPU_DESCRIPTOR_HANDLE dest)
{
    if (device == nullptr || resource == nullptr || sizeInBytes == 0)
    {
        return false;
    }

    uint32_t numElements = 0;
    uint32_t stride = 0;

    if (isRawBuffer)
    {
        // Raw 视图以 32 位元素为单位，字节数需按 4 字节对齐
        numElements = (uint32_t)(sizeInBytes / 4);
    }
    else
    {
        stride = structuredStride;
        if (stride == 0)
        {
            LOG_ERROR("[DX12] 创建结构化缓冲视图失败：缺少元素步长"
                      "（StructuredBuffer 在 D3D12 下必须显式给出 StructStride）。size=%llu",
                      (unsigned long long)sizeInBytes);
            return false;
        }
        numElements = (uint32_t)(sizeInBytes / stride);
    }

    if (numElements == 0)
    {
        LOG_ERROR("[DX12] 创建缓冲视图失败：元素数为 0（size=%llu, stride=%u, raw=%d）",
                  (unsigned long long)sizeInBytes, stride, (int)isRawBuffer);
        return false;
    }

    if (asUAV)
    {
        const D3D12_UNORDERED_ACCESS_VIEW_DESC desc = DX12BufferUAVDesc(0, numElements, stride);
        device->CreateUnorderedAccessView(resource, nullptr, &desc, dest);
    }
    else
    {
        const D3D12_SHADER_RESOURCE_VIEW_DESC desc = DX12BufferSRVDesc(0, numElements, stride);
        device->CreateShaderResourceView(resource, &desc, dest);
    }

    return true;
}

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_HELPERS_INCLUDE_JHGDS */
