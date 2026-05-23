//
//  VirtualTexturePageTable.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTexturePageTable.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

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

    mGPUTableTexture = RenderCore::GetRenderDevice()->CreateTexture2D(RenderCore::kTexFormatR32Uint,
        RenderCore::TextureUsage::TextureUsageShaderRead, mGridWidth[0], mGridHeight[0], mMipLevels);
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
	for (size_t i = 0; i < mCPUTables.size(); ++i)
    {
		RenderCore::Rect2D region;
		region.offsetX = 0;
		region.offsetY = 0;
		region.width = mGridWidth[i];
		region.height = mGridHeight[i];

        mGPUTableTexture->ReplaceRegion(region, i, (const uint8_t*)mCPUTables[i].data(), region.width * 4);
	}
}

NS_RENDERSYSTEM_END
