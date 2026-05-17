//
//  VirtualTexturePageTable.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTexturePageTable.h"

NS_RENDERSYSTEM_BEGIN

VirtualTexturePageTable::VirtualTexturePageTable(const VirtualTextureConfig& config)
    : mVirtualWidth(config.virtualWidth)
    , mVirtualHeight(config.virtualHeight)
    , mPageSize(config.pageSize)
    , mMipLevels(config.mipLevels)
{
    mGridWidth.resize(mMipLevels);
    mGridHeight.resize(mMipLevels);
    mCPUTables.resize(mMipLevels);

    for (uint32_t mip = 0; mip < mMipLevels; ++mip)
    {
        mGridWidth[mip]  = GetPageGridCount(mVirtualWidth,  mip);
        mGridHeight[mip] = GetPageGridCount(mVirtualHeight, mip);
        mCPUTables[mip].resize(
            static_cast<size_t>(mGridWidth[mip]) * mGridHeight[mip], 0
        );
    }

    // TODO: 创建 GPU R32UI 纹理（mip chain = mMipLevels）。
}

void VirtualTexturePageTable::WriteEntry(const PageRequest& request, PageTableEntry entry)
{
    mCPUTables[request.mipLevel][LinearIndex(mGridWidth[request.mipLevel], request.pageX, request.pageY)] = entry;
}

bool VirtualTexturePageTable::IsResident(const PageRequest& request) const
{
    return (mCPUTables[request.mipLevel][LinearIndex(mGridWidth[request.mipLevel], request.pageX, request.pageY)] & 1) != 0;
}

PageTableEntry VirtualTexturePageTable::ReadEntry(const PageRequest& request) const
{
    return mCPUTables[request.mipLevel][LinearIndex(mGridWidth[request.mipLevel], request.pageX, request.pageY)];
}

void VirtualTexturePageTable::ClearEntry(const PageRequest& request)
{
    mCPUTables[request.mipLevel][LinearIndex(mGridWidth[request.mipLevel], request.pageX, request.pageY)] &= ~1u;
}

void VirtualTexturePageTable::SyncToGPU()
{
    // TODO: 遍历每级 mip，将 dirty 或全部 page table 数据上传到 mGPUTableTexture。
}

NS_RENDERSYSTEM_END