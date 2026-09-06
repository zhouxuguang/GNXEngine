//
//  AssetPackHelpers.h
//  GNXEngine
//
//  AssetProcess 内部共享的资产打包辅助函数（不对外暴露，仅供
//  ModelAssetPackager / EnvironmentAssetBaker 两个 .cpp 复用）。
//

#ifndef GNX_ENGINE_ASSET_PACK_HELPERS_INCLUDE
#define GNX_ENGINE_ASSET_PACK_HELPERS_INCLUDE

#include "AssetProcessDefine.h"
#include "Runtime/AssetManager/include/AssetContainerWriter.h"
#include "Runtime/AssetManager/include/TextureMessageUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

NS_ASSETPROCESS_BEGIN

namespace AssetPackHelpers
{

// 从文件路径取 stem（无扩展名文件名），用作默认资产名
inline std::string GetStemName(const std::string& path)
{
    return fs::path(path).stem().string();
}

// 把 KTX 字节编码为 TextureMessage pb，再写成 .texture 容器
// （.texture 载荷必须是 TextureMessage pb；运行时经 DecodeTextureMessage 还原 KTX）
inline bool WriteTextureContainer(const std::string& outPath,
                                  const std::vector<uint8_t>& ktxData,
                                  const std::string& assetName,
                                  uint32_t flags = AssetManager::AssetFileFlags::NONE)
{
    ByteVectorPtr encoded = AssetManager::TextureMessageUtil::EncodeTextureMessage(ktxData.data(),
                                                                                   (uint32_t)ktxData.size());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("AssetPackHelpers: TextureMessage encode failed for %s", outPath.c_str());
        return false;
    }
    std::vector<uint8_t> pbData(encoded->begin(), encoded->end());
    return AssetManager::AssetContainerWriter::WriteAssetFile(
        outPath, AssetManager::AssetType::Texture, assetName, pbData, flags);
}

} // namespace AssetPackHelpers

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSET_PACK_HELPERS_INCLUDE
