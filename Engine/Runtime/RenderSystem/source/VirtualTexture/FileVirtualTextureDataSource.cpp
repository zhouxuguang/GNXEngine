//
//  FileVirtualTextureDataSource.cpp
//  GNXEngine
//

#include "VirtualTexture/FileVirtualTextureDataSource.h"
#include "Runtime/BaseLib/include/FileUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include <vector>

NS_RENDERSYSTEM_BEGIN

namespace
{

/// 把解码后的任意 PNG 像素格式统一转换为 RGBA8（虚拟纹理 atlas 的固定格式）。
/// 返回空 vector 表示格式不支持或数据非法。
std::vector<uint8_t> ConvertToRGBA8(const imagecodec::VImage& image)
{
    const uint32_t width  = image.GetWidth();
    const uint32_t height = image.GetHeight();
    const uint32_t bpp    = image.GetBytesPerPixels();
    const uint8_t* src    = image.GetImageData();
    const uint32_t srcRow = image.GetBytesPerRow();

    if (src == nullptr || width == 0 || height == 0 || bpp == 0)
    {
        return {};
    }

    std::vector<uint8_t> out(static_cast<size_t>(width) * height * 4u);
    for (uint32_t y = 0; y < height; ++y)
    {
        const uint8_t* s = src + static_cast<size_t>(y) * srcRow;
        uint8_t* d = out.data() + static_cast<size_t>(y) * width * 4u;

        for (uint32_t x = 0; x < width; ++x)
        {
            switch (bpp)
            {
                case 4:
                    d[x * 4 + 0] = s[x * 4 + 0];
                    d[x * 4 + 1] = s[x * 4 + 1];
                    d[x * 4 + 2] = s[x * 4 + 2];
                    d[x * 4 + 3] = s[x * 4 + 3];
                    break;
                case 3:
                    d[x * 4 + 0] = s[x * 3 + 0];
                    d[x * 4 + 1] = s[x * 3 + 1];
                    d[x * 4 + 2] = s[x * 3 + 2];
                    d[x * 4 + 3] = 255;
                    break;
                case 2:
                    d[x * 4 + 0] = s[x * 2 + 0];
                    d[x * 4 + 1] = s[x * 2 + 0];
                    d[x * 4 + 2] = s[x * 2 + 0];
                    d[x * 4 + 3] = s[x * 2 + 1];
                    break;
                case 1:
                    d[x * 4 + 0] = s[x];
                    d[x * 4 + 1] = s[x];
                    d[x * 4 + 2] = s[x];
                    d[x * 4 + 3] = 255;
                    break;
                default:
                    return {};
            }
        }
    }

    return out;
}

} // namespace

FileVirtualTextureDataSource::FileVirtualTextureDataSource(std::string basePath, 
                                                           std::string extension)
    : mBasePath(std::move(basePath))
    , mExtension(std::move(extension))
{
    // 确保 basePath 末尾有路径分隔符
    if (!mBasePath.empty() && mBasePath.back() != '/' && mBasePath.back() != '\\')
    {
        mBasePath += '/';
    }

    // 统一扩展名格式：允许调用方传 "png" 或 ".png"，内部统一带上点号
    if (!mExtension.empty() && mExtension.front() != '.')
    {
        mExtension.insert(mExtension.begin(), '.');
    }
}

std::string FileVirtualTextureDataSource::BuildTilePath(const PageRequest& page) const
{
    // 约定：{basePath}/{mipLevel}_{pageX}_{pageY}{ext}
    // 与 vtile 离线切图产出的文件名一致（例如 "0_3_5.png" 表示 mip0 的 (3,5)）。
    std::string path = mBasePath;
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
    return std::async(std::launch::async, [this, page]() -> std::vector<uint8_t>
    {
        const std::string path = BuildTilePath(page);

        std::vector<uint8_t> fileData = baselib::FileUtil::ReadBinaryFile(path);
        if (fileData.empty())
        {
            LOG_WARN("VT: tile file not found or empty: %s", path.c_str());
            return {};
        }

        // tile 文件是 PNG，需要解码为 RGBA8 像素后交给 atlas 上传
        imagecodec::VImage image;
        if (!imagecodec::ImageDecoder::DecodeMemory(fileData.data(), fileData.size(), &image))
        {
            LOG_ERROR("VT: failed to decode tile image: %s", path.c_str());
            return {};
        }

        return ConvertToRGBA8(image);
    });
}

NS_RENDERSYSTEM_END
