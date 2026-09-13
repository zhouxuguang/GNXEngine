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
    , mTotalMipLevels(config.mipLevels > 0 ? config.mipLevels : 1)
{
    // 常驻的是“最粗糙”的若干级 mip：mipLevel ∈ [mTotalMipLevels - mPinnedMipLevels, mTotalMipLevels)
    if (mPinnedMipLevels >= mTotalMipLevels)
    {
        mMinPinnedMip = 0;
    }
    else
    {
        mMinPinnedMip = mTotalMipLevels - mPinnedMipLevels;
    }

    mFreeSlots.reserve(static_cast<size_t>(mAtlasSlotsX) * mAtlasSlotsY);
    for (uint32_t y = 0; y < mAtlasSlotsY; ++y)
    {
        for (uint32_t x = 0; x < mAtlasSlotsX; ++x)
        {
            mFreeSlots.push_back({x, y});
        }
    }
}

CacheAllocation VirtualTextureCache::Allocate(
    const PageRequest& request,
    const std::unordered_set<PageRequest>* protectedRequests)
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
    const std::optional<PageRequest> evicted = FindEvictionCandidate(protectedRequests);
    if (!evicted)
    {
        // 所有 page 都被 pinned，没有可淘汰的
        return {false, {}, false, {}};
    }
    PageSlot slot = mActiveAllocations.at(*evicted);
    Evict(*evicted);
    mActiveAllocations.erase(*evicted);
    mSlotOwners.erase(slot);

    return {true, slot, true, *evicted};
}

void VirtualTextureCache::Touch(const PageRequest& request)
{
    if (IsPinnedLod(request.mipLevel))
    {
        return; // 常驻页不参与 LRU，无需更新
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

    // 常驻页不会被淘汰，不需要加入 LRU
    if (!IsPinnedLod(request.mipLevel))
    {
        mLRUList.push_front(request);
        mLRUMap[request] = mLRUList.begin();
    }
}

void VirtualTextureCache::FreeSlot(const PageSlot& slot)
{
    mFreeSlots.push_back(slot);
}

std::optional<PageRequest> VirtualTextureCache::FindEvictionCandidate(
    const std::unordered_set<PageRequest>* protectedRequests) const
{
    // 从 LRU 尾部（最久未使用）向前遍历。本帧 feedback 仍在使用的
    // page 不能被淘汰，否则工作集大于 atlas 时会在相邻帧间循环换页。
    for (auto it = mLRUList.rbegin(); it != mLRUList.rend(); ++it)
    {
        if (!IsPinnedLod(it->mipLevel) &&
            (!protectedRequests || protectedRequests->find(*it) == protectedRequests->end()))
        {
            return *it;
        }
    }
    return std::nullopt;
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
