//
//  VirtualTextureCache.cpp
//  GNXEngine
//

#include "VirtualTexture/VirtualTextureCache.h"

NS_RENDERSYSTEM_BEGIN

VirtualTextureCache::VirtualTextureCache(const VirtualTextureConfig& config)
    : mAtlasSlotsX(config.atlasSlotsX)
    , mAtlasSlotsY(config.atlasSlotsY)
    , mSlotSize(config.slotSize)
    , mPinnedMipLevels(config.pinnedMipLevels)
    , mUploadsPerFrame(config.uploadsPerFrame)
{
    mFreeSlots.reserve(static_cast<size_t>(mAtlasSlotsX) * mAtlasSlotsY);
    for (uint32_t y = 0; y < mAtlasSlotsY; ++y)
    {
        for (uint32_t x = 0; x < mAtlasSlotsX; ++x)
        {
            mFreeSlots.push_back({x, y});
        }
    }
}

CacheAllocation VirtualTextureCache::Allocate(const PageRequest& request)
{
    if (mActiveAllocations.find(request) != mActiveAllocations.end())
    {
        return {true, mActiveAllocations[request], false, {}};
    }

    if (!mFreeSlots.empty())
    {
        PageSlot slot = mFreeSlots.back();
        mFreeSlots.pop_back();
        return {true, slot, false, {}};
    }

    // LRU 淘汰
    PageRequest evicted = FindEvictionCandidate();
    Evict(evicted);
    PageSlot slot = mActiveAllocations[evicted];
    mActiveAllocations.erase(evicted);
    mSlotOwners.erase(slot);

    return {true, slot, true, evicted};
}

void VirtualTextureCache::Touch(const PageRequest& request)
{
    // TODO: 将 request 移到 mLRUList 头部。
}

void VirtualTextureCache::Commit(const PageRequest& request, const PageSlot& slot)
{
    mActiveAllocations[request] = slot;
    mSlotOwners[slot] = request;
    mLRUList.push_front(request);
    mLRUMap[request] = mLRUList.begin();
}

void VirtualTextureCache::FreeSlot(const PageSlot& slot)
{
    mFreeSlots.push_back(slot);
}

PageRequest VirtualTextureCache::FindEvictionCandidate() const
{
    // TODO: 从 mLRUList 末尾查找第一个 mipLevel >= mPinnedMipLevels 的 page 返回。
    return { 0, 0, 0 };
}

void VirtualTextureCache::Evict(const PageRequest& request)
{
    // TODO: 清除 page table 中的 resident 标记。
    // TODO: 从 LRU 列表中移除。
}

NS_RENDERSYSTEM_END
