//
//  FileVirtualTextureDataSource.h
//  GNXEngine
//
//  本地文件虚拟纹理数据源。
//  从已切片好的虚拟纹理文件中按需读取 tile 数据。
//  文件命名约定：{basePath}/mip{mipLevel}/{pageX}_{pageY}.{ext}
//  例如：Textures/Terrain/mip2/5_8.tile
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
    /// @param extension  文件扩展名（包含点号，默认 ".tile"）
    explicit FileVirtualTextureDataSource(std::string basePath, 
                                          std::string extension = ".tile");

    std::future<std::vector<uint8_t>> RequestTile(const PageRequest& page) override;

private:
    std::string mBasePath;
    std::string mExtension;

    /// 根据 PageRequest 构建完整的 tile 文件路径。
    /// 格式：{basePath}/mip{mipLevel}/{pageX}_{pageY}.{ext}
    std::string BuildTilePath(const PageRequest& page) const;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_VIRTUALTEXTURE_FILEDATASOURCE_H */