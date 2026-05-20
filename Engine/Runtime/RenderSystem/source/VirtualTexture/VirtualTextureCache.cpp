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
    if (mActiveAllocations.find(evicted) == mActiveAllocations.end())
    {
        // 所有 page 都被 pinned，没有可淘汰的
        return {false, {}, false, {}};
    }
    Evict(evicted);
    PageSlot slot = mActiveAllocations[evicted];
    mActiveAllocations.erase(evicted);
    mSlotOwners.erase(slot);

    return {true, slot, true, evicted};
}

void VirtualTextureCache::Touch(const PageRequest& request)
{
    if (request.mipLevel >= mPinnedMipLevels)
    {
        return; // no-op for pinned lods
    }

	if (auto it = mLRUMap.find(request); it != mLRUMap.end())
    {
        mLRUList.splice(mLRUList.begin(), mLRUList, it->second);
	}
}

void VirtualTextureCache::Commit(const PageRequest& request, const PageSlot& slot)
{
    // Allocate() 已确保 request 未被分配，直接写入映射并加入 LRU
    mActiveAllocations[request] = slot;
    mSlotOwners[slot] = request;

    // pinned page 不会被淘汰，不需要加入 LRU
    if (request.mipLevel < mPinnedMipLevels)
    {
        mLRUList.push_front(request);
        mLRUMap[request] = mLRUList.begin();
    }
}

void VirtualTextureCache::FreeSlot(const PageSlot& slot)
{
    mFreeSlots.push_back(slot);
}

PageRequest VirtualTextureCache::FindEvictionCandidate() const
{
    // 从 LRU 尾部（最久未使用）向前遍历，找到第一个非 pinned 的 page
    for (auto it = mLRUList.rbegin(); it != mLRUList.rend(); ++it)
    {
        if (it->mipLevel < mPinnedMipLevels)
        {
            return *it;
        }
    }
    // 所有 page 都被 pinned，调用方 Allocate 通过 mActiveAllocations 检查兜底
    return {0, 0, 0};
}

void VirtualTextureCache::Evict(const PageRequest& request)
{
    // 从 LRU 索引表中找到链表节点迭代器，删除节点并清除索引
    auto it = mLRUMap.find(request);
    if (it != mLRUMap.end())
    {
        mLRUList.erase(it->second);
        mLRUMap.erase(it);
    }
    // mActiveAllocations 和 mSlotOwners 由调用方 Allocate 自行清理
}

NS_RENDERSYSTEM_END
