//
//  VirtualTextureDefines.h
//  GNXEngine
//
//  Virtual Texture 系统的基础数据结构与可配置参数定义。
//  可配置参数集中到 VirtualTextureConfig，运行时通过
//  VirtualTextureManager::Initialize() 传入并分发到各组件。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DEFINES_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DEFINES_H

#include "../RSDefine.h"
#include <cstdint>

NS_RENDERSYSTEM_BEGIN

/// FeedbackPixel / PageTableEntry 编码格式：
///   bit 0   : resident 标记
///   bit 1-8 : atlas slot X（8 位）
///   bit 9-16: atlas slot Y（8 位）
///   bit 31  : valid 标记（仅 feedback）

/// Page table entry 编码帮助函数。
inline uint32_t EncodePageTableEntry(uint32_t slotX, uint32_t slotY)
{
    return 1u | ((slotX & 0xFFu) << 1) | ((slotY & 0xFFu) << 9);
}

inline uint32_t GetSlotXFromEntry(uint32_t entry) { return (entry >> 1) & 0xFFu; }
inline uint32_t GetSlotYFromEntry(uint32_t entry) { return (entry >> 9) & 0xFFu; }

// ──────────────────────────────────────────────
//  运行时配置
// ──────────────────────────────────────────────

/// Virtual Texture 系统的运行时参数。
/// 通过 VirtualTextureManager::Initialize() 传入。
struct VirtualTextureConfig
{
    // ── 虚拟纹理尺寸 ──
    uint32_t virtualWidth  = 65536;
    uint32_t virtualHeight = 65536;

    // ── Tile 参数 ──
    uint32_t pageSize   = 128;   // 每个 tile 的像素尺寸
    uint32_t pageBorder = 4;     // tile 边距（用于滤波）

    // ── 物理 atlas 规模 ──
    uint32_t atlasSlotsX = 32;   // 水平方向 slot 数
    uint32_t atlasSlotsY = 32;

    // ── 常驻页 ──
    uint32_t pinnedMipLevels = 2; // 最低几级 mip 常驻不淘汰

    // ── 每帧上传限制 ──
    uint32_t uploadsPerFrame = 8; // 每帧最多上传 N 个 tile 到 GPU

    // ── 派生属性（由 Init 填充，使用者先不用手填） ──
    uint32_t slotSize   = 0;  // = pageSize + pageBorder * 2
    uint32_t atlasWidth = 0;  // = atlasSlotsX * slotSize
    uint32_t atlasHeight = 0; // = atlasSlotsY * slotSize
    uint32_t mipLevels  = 0;  // = floor(log2(min(virtualW, virtualH) / pageSize)) + 1

    /// 用当前值填充所有派生字段。
    void ResolveDerived()
    {
        slotSize   = pageSize + pageBorder * 2;
        atlasWidth = atlasSlotsX * slotSize;
        atlasHeight = atlasSlotsY * slotSize;

        uint32_t minDim = (virtualWidth < virtualHeight) ? virtualWidth : virtualHeight;
        uint32_t pagesPerDim = minDim / pageSize;
        mipLevels = 0;
        while (pagesPerDim > 0) 
        {
            ++mipLevels;
            pagesPerDim >>= 1;
        }
        if (mipLevels < 1) 
        {
            mipLevels = 1;
        }
    }
};

// ──────────────────────────────────────────────
//  数据结构
// ──────────────────────────────────────────────

/// Page 请求标识：虚拟纹理中的某个 tile。
struct PageRequest
{
    uint32_t mipLevel;
    uint32_t pageX;
    uint32_t pageY;

    bool operator==(const PageRequest& other) const
    {
        return mipLevel == other.mipLevel && pageX == other.pageX && pageY == other.pageY;
    }
};

/// Page 在物理 atlas 中的位置。
struct PageSlot
{
    uint32_t atlasX;
    uint32_t atlasY;

    bool operator==(const PageSlot& other) const
    {
        return atlasX == other.atlasX && atlasY == other.atlasY;
    }
};

using PageTableEntry = uint32_t;
using FeedbackPixel  = uint32_t;

/// 获取指定 mip 层级的 tile 网格维度。
inline uint32_t GetPageGridCount(uint32_t dimension, uint32_t mipLevel)
{
    return (dimension >> mipLevel) > 0 ? (dimension >> mipLevel) : 1;
}

/// Page table / feedback 的内存开销估算（用于调试/调参）。
inline uint64_t EstimatePageTableMemory(const VirtualTextureConfig& cfg)
{
    // 每级 mip 的 page 数量 × 4 bytes
    uint64_t total = 0;
    for (uint32_t mip = 0; mip < cfg.mipLevels; ++mip)
    {
        uint32_t w = GetPageGridCount(cfg.virtualWidth,  mip);
        uint32_t h = GetPageGridCount(cfg.virtualHeight, mip);
        total += static_cast<uint64_t>(w) * h;
    }
    return total * sizeof(PageTableEntry);
}

inline uint64_t EstimateAtlasMemory(const VirtualTextureConfig& cfg)
{
    // SRGBA8 4 字节每像素
    return static_cast<uint64_t>(cfg.atlasWidth) * cfg.atlasHeight * 4;
}

NS_RENDERSYSTEM_END

// ──────────────────────────────────────────────
//  std::hash 特化
// ──────────────────────────────────────────────

namespace std 
{

template<>
struct hash<RenderSystem::PageRequest>
{
    size_t operator()(const RenderSystem::PageRequest& r) const noexcept
    {
        return hash<uint64_t>{}(
            (static_cast<uint64_t>(r.mipLevel) << 42) ^
            (static_cast<uint64_t>(r.pageX)    << 21) ^
            static_cast<uint64_t>(r.pageY)
        );
    }
};

template<>
struct hash<RenderSystem::PageSlot>
{
    size_t operator()(const RenderSystem::PageSlot& s) const noexcept
    {
        return hash<uint64_t>{}(
            (static_cast<uint64_t>(s.atlasX) << 32) |
            static_cast<uint64_t>(s.atlasY)
        );
    }
};

} // namespace std

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DEFINES_H */
