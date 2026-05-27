//
//  FileVirtualTextureDataSource.cpp
//  GNXEngine
//

#include "VirtualTexture/FileVirtualTextureDataSource.h"
#include "Runtime/BaseLib/include/FileUtil.h"

NS_RENDERSYSTEM_BEGIN

FileVirtualTextureDataSource::FileVirtualTextureDataSource(std::string basePath, 
                                                           std::string extension)
    : mBasePath(std::move(basePath))
    , mExtension(std::move(extension))
{
    // 确保 basePath 末尾没有多余的路径分隔符
    if (!mBasePath.empty() && mBasePath.back() != '/' && mBasePath.back() != '\\')
    {
        mBasePath += '/';
    }
}

std::string FileVirtualTextureDataSource::BuildTilePath(const PageRequest& page) const
{
    // 格式：{basePath}/mip{mipLevel}/{pageX}_{pageY}.{ext}
    //std::format("assets/pages/{}_{}_{}.png", request.lod, request.x, request.y)
    std::string path = mBasePath;
    path += "mip";
    path += std::to_string(page.mipLevel);
    path += '_';
    path += std::to_string(page.pageX);
    path += '_';
    path += std::to_string(page.pageY);
    path += mExtension;
    return path;
}

std::future<std::vector<uint8_t>> FileVirtualTextureDataSource::RequestTile(const PageRequest& page)
{
    return std::async(std::launch::async, [this, page]()
    {
        std::string path = BuildTilePath(page);
        return baselib::FileUtil::ReadBinaryFile(path);
    });
}

NS_RENDERSYSTEM_END