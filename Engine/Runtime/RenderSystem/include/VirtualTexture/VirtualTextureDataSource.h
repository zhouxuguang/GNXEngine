//
//  VirtualTextureDataSource.h
//  GNXEngine
//
//  虚拟纹理 tile 数据源抽象接口。
//  不同实现支持从文件、网络、内存等来源加载 tile 数据。
//  使用者通过 VirtualTextureManager::Initialize() 传入。
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DATASOURCE_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DATASOURCE_H

#include "VirtualTextureDefines.h"
#include <future>
#include <vector>
#include <cstdint>

NS_RENDERSYSTEM_BEGIN

/// 虚拟纹理 tile 数据源抽象接口。
/// 实现者需要提供异步加载单个 tile 的能力。
class IVirtualTextureDataSource
{
public:
    virtual ~IVirtualTextureDataSource() = default;

    /// 异步加载指定 tile 的 RGBA8 像素数据。
    /// @param page 要加载的 tile 标识（mipLevel, pageX, pageY）
    /// @return 返回一个 future，ready 时包含 tile 的 RGBA8 像素数据
    virtual std::future<std::vector<uint8_t>> RequestTile(const PageRequest& page) = 0;
};

/// 空的 tile 数据源（占位用，始终返回空数据）。
/// 用于测试或尚未接入真实数据源的场景。
class NullVirtualTextureDataSource : public IVirtualTextureDataSource
{
public:
    std::future<std::vector<uint8_t>> RequestTile(const PageRequest& /*page*/) override
    {
        std::promise<std::vector<uint8_t>> promise;
        promise.set_value({});
        return promise.get_future();
    }
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_DATASOURCE_H */