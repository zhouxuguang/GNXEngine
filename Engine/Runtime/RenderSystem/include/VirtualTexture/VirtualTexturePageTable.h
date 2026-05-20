//
//  VirtualTexturePageTable.h
//  GNXEngine
//
//  多级页表：维护虚拟 tile → 物理 slot 的映射关系。
//  每级 mip 一张 page table，存储在 GPU R32UI 纹理中。
//  Page table 的虚拟纹理尺寸等参数通过 VirtualTextureConfig 传入。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_PAGETABLE_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_PAGETABLE_H

#include "VirtualTextureDefines.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include <vector>

NS_RENDERSYSTEM_BEGIN

/// 多级 page table 管理器。
class RENDERSYSTEM_API VirtualTexturePageTable
{
public:
    /// 构造时需传入配置，page table 尺寸由 config 的虚拟纹理尺寸和 page 尺寸推导。
    explicit VirtualTexturePageTable(const VirtualTextureConfig& config);
    ~VirtualTexturePageTable() = default;

    VirtualTexturePageTable(const VirtualTexturePageTable&) = delete;
    VirtualTexturePageTable& operator=(const VirtualTexturePageTable&) = delete;

    /// 写入一个 entry（标记 resident 状态及物理 slot 位置）。
    void WriteEntry(const PageRequest& request, PageTableEntry entry);

    /// 查询指定 tile 是否已 resident。
    bool IsResident(const PageRequest& request) const;

    /// 读取 entry（用于 eviction 时获取物理 slot 位置）。
    PageTableEntry ReadEntry(const PageRequest& request) const;

    /// 清除指定 tile 的 resident 标记（eviction 时调用）。
    void ClearEntry(const PageRequest& request);

    /// 将所有脏 page table 数据上传到 GPU 纹理。
    void SyncToGPU();

    /// 获取 GPU page table 纹理。
    const RCTexturePtr& GetGPUTexture() const { return mGPUTableTexture; }

    /// 获取 page table 的 mip 层级数。
    uint32_t GetMipLevels() const { return mMipLevels; }
    uint32_t GetGridWidth(uint32_t mip) const { return mGridWidth[mip]; }
    uint32_t GetGridHeight(uint32_t mip) const { return mGridHeight[mip]; }

private:
    // CPU 端 page table 副本：mCPUTables[mip] 是该级 mip 的 entry 数组。
    std::vector<std::vector<PageTableEntry>> mCPUTables;

    // GPU 端：R32UI 纹理，使用 mip chain 存储各级 page table。
    RCTexture2DPtr mGPUTableTexture;

    // 配置缓存
    uint32_t mVirtualWidth  = 0;
    uint32_t mVirtualHeight = 0;
    uint32_t mPageSize      = 0;
    uint32_t mMipLevels     = 0;

    // 每级 mip 的 tile 网格尺寸（运行时由 Config 计算）。
    std::vector<uint32_t> mGridWidth;
    std::vector<uint32_t> mGridHeight;

    /// 将 (pageX, pageY) 线性化到宽度为 mapWidth 的数组中。
    static size_t LinearIndex(uint32_t mapWidth, uint32_t pageX, uint32_t pageY)
    {
        return static_cast<size_t>(pageY) * mapWidth + pageX;
    }
};

using VirtualTexturePageTablePtr = std::shared_ptr<VirtualTexturePageTable>;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_PAGETABLE_H */