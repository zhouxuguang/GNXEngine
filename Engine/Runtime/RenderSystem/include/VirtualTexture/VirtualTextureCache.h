//
//  VirtualTextureCache.h
//  GNXEngine
//
//  物理 atlas 缓存管理：维护 GPU 上物理 tile 池的分配与回收。
//  使用 LRU 淘汰策略，支持 pinned page（低 mip 常驻不淘汰）。
//  物理 atlas 尺寸等参数通过 VirtualTextureConfig 传入。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_CACHE_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_CACHE_H

#include "VirtualTextureDefines.h"
#include <vector>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

NS_RENDERSYSTEM_BEGIN

// ──────────────────────────────────────────────
//  物理 atlas 缓存分配结果
// ──────────────────────────────────────────────

struct CacheAllocation
{
    bool     success;
    PageSlot slot;
    bool     hasEvicted;
    PageRequest evictedRequest;
};

// ──────────────────────────────────────────────
//  物理 atlas 缓存
// ──────────────────────────────────────────────

class RENDERSYSTEM_API VirtualTextureCache
{
public:
    explicit VirtualTextureCache(const VirtualTextureConfig& config);
    ~VirtualTextureCache() = default;

    VirtualTextureCache(const VirtualTextureCache&) = delete;
    VirtualTextureCache& operator=(const VirtualTextureCache&) = delete;

    /// 请求为指定 page 分配物理 slot。
    CacheAllocation Allocate(const PageRequest& request,
                             const std::unordered_set<PageRequest>* protectedRequests = nullptr);

    /// 标记指定 page 最近被使用（更新 LRU 顺序）。
    void Touch(const PageRequest& request);

    /// 提交分配：将 PageRequest 绑定到 PageSlot。
    void Commit(const PageRequest& request, const PageSlot& slot);

    /// 释放指定 page 占用的物理 slot。
    void FreeSlot(const PageSlot& slot);

    /// 每帧最大上传数。
    void SetUploadsPerFrame(uint32_t count) { mUploadsPerFrame = count; }
    uint32_t GetUploadsPerFrame() const { return mUploadsPerFrame; }

    /// 获取 config 参数的访问接口（供 Manager 创建纹理时参考）。
    uint32_t GetAtlasSlotsX() const { return mAtlasSlotsX; }
    uint32_t GetAtlasSlotsY() const { return mAtlasSlotsY; }
    uint32_t GetSlotSize()   const { return mSlotSize; }

    /// 当前已占用的 slot 数量（= resident page 数量，用于调试显示）。
    uint32_t GetResidentCount() const { return static_cast<uint32_t>(mActiveAllocations.size()); }

    /// 物理 atlas 的总 slot 容量。
    uint32_t GetCapacity() const { return mAtlasSlotsX * mAtlasSlotsY; }

    /// 判断某个 mip 是否为常驻（不可淘汰）层级。
    /// 常驻层级是最粗糙的若干级 mip（mipLevel >= mTotalMipLevels - mPinnedMipLevels），
    /// 它们提供 mip 回退的兜底数据。
    bool IsPinnedLod(uint32_t mipLevel) const { return mipLevel >= mMinPinnedMip; }

private:
    // 配置缓存
    uint32_t mAtlasSlotsX = 0;
    uint32_t mAtlasSlotsY = 0;
    uint32_t mSlotSize    = 0;
    uint32_t mPinnedMipLevels = 0;
    uint32_t mUploadsPerFrame = 8;
    uint32_t mTotalMipLevels  = 1;
    uint32_t mMinPinnedMip    = 0;   // 常驻层级的下界

    // 空闲 slot 池。
    std::vector<PageSlot> mFreeSlots;

    // LRU 列表
    std::list<PageRequest> mLRUList;
    std::unordered_map<PageRequest, std::list<PageRequest>::iterator> mLRUMap;

    // 分配记录
    std::unordered_map<PageRequest, PageSlot> mActiveAllocations;
    std::unordered_map<PageSlot, PageRequest> mSlotOwners;

    /// 查找最久未使用的可淘汰 page。
    std::optional<PageRequest> FindEvictionCandidate(
        const std::unordered_set<PageRequest>* protectedRequests) const;

    /// 执行淘汰。
    void Evict(const PageRequest& request);
};

using VirtualTextureCachePtr = std::shared_ptr<VirtualTextureCache>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_CACHE_H */
