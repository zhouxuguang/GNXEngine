//
//  FileVirtualTextureDataSource.h
//  GNXEngine
//
//  本地文件虚拟纹理数据源。
//  从已切片好的虚拟纹理文件中按需读取 tile 数据，并解码为 RGBA8 像素。
//  文件命名约定：{basePath}/{mipLevel}_{pageX}_{pageY}{ext}
//  例如：data_asset/vt/pages/2_5_8.png
//

#ifndef GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FILEDATASOURCE_H
#define GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FILEDATASOURCE_H

#include "VirtualTextureDataSource.h"
#include <string>

NS_RENDERSYSTEM_BEGIN

/// 本地文件虚拟纹理数据源。
/// 从已按 mip 层级和 tile 坐标切片好的文件中异步加载 tile 数据。
class RENDERSYSTEM_API FileVirtualTextureDataSource : public IVirtualTextureDataSource
{
public:
    /// @param basePath  切片文件所在根目录路径
    /// @param extension  文件扩展名（可带点号也可不带，默认 ".tile"）
    explicit FileVirtualTextureDataSource(std::string basePath, 
                                          std::string extension = ".tile");

    /// 异步读取并解码一个 tile，返回 RGBA8 像素数据（失败返回空）。
    std::future<std::vector<uint8_t>> RequestTile(const PageRequest& page) override;

private:
    std::string mBasePath;
    std::string mExtension;

    /// 根据 PageRequest 构建完整的 tile 文件路径。
    /// 格式：{basePath}/{mipLevel}_{pageX}_{pageY}{ext}
    std::string BuildTilePath(const PageRequest& page) const;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FILEDATASOURCE_H */