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
#include "Runtime/RenderCore/include/RCTexture.h"
#include <vector>
#include <list>
#include <unordered_map>

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
    CacheAllocation Allocate(const PageRequest& request);

    /// 标记指定 page 最近被使用（更新 LRU 顺序）。
    void Touch(const PageRequest& request);

    /// 提交分配：将 PageRequest 绑定到 PageSlot。
    void Commit(const PageRequest& request, const PageSlot& slot);

    /// 释放指定 page 占用的物理 slot。
    void FreeSlot(const PageSlot& slot);

    /// 获取物理 atlas 纹理。
    const RCTexture2DPtr& GetAtlasTexture() const { return mAtlasTexture; }

    /// 每帧最大上传数。
    void SetUploadsPerFrame(uint32_t count) { mUploadsPerFrame = count; }
    uint32_t GetUploadsPerFrame() const { return mUploadsPerFrame; }

private:
    // 配置缓存
    uint32_t mAtlasSlotsX = 0;
    uint32_t mAtlasSlotsY = 0;
    uint32_t mSlotSize    = 0;
    uint32_t mPinnedMipLevels = 0;
    uint32_t mUploadsPerFrame = 8;

    // GPU 物理 atlas 纹理。
    RCTexture2DPtr mAtlasTexture;

    // 空闲 slot 池。
    std::vector<PageSlot> mFreeSlots;

    // LRU 列表
    std::list<PageRequest> mLRUList;
    std::unordered_map<PageRequest, std::list<PageRequest>::iterator> mLRUMap;

    // 分配记录
    std::unordered_map<PageRequest, PageSlot> mActiveAllocations;
    std::unordered_map<PageSlot, PageRequest> mSlotOwners;

    /// 查找最久未使用的可淘汰 page。
    PageRequest FindEvictionCandidate() const;

    /// 执行淘汰。
    void Evict(const PageRequest& request);
};

using VirtualTextureCachePtr = std::shared_ptr<VirtualTextureCache>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_CACHE_H */