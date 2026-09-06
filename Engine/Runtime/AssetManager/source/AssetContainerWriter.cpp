//
//  AssetContainerWriter.cpp
//  GNXEngine
//
//  通用资产容器写入器实现。
//  与 ShaderPackageBuilder::Save / TextureImporter::SaveTextureFile 的写盘
//  逻辑同构，但入参为"pb 字节 + AssetType + 输出路径"，可复用于任意资产类型。
//

#include "AssetContainerWriter.h"
#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

NS_ASSETMANAGER_BEGIN

bool AssetContainerWriter::WriteAssetFile(const std::string& outPath,
                                          AssetType type,
                                          const std::string& assetName,
                                          const uint8_t* pbData,
                                          size_t pbSize,
                                          uint32_t flags)
{
    if (!pbData || pbSize == 0)
    {
        LOG_ERROR("AssetContainerWriter: empty payload for %s", outPath.c_str());
        return false;
    }

    // 计算载荷 hash 并创建文件头
    uint64_t hash = AssetFileHeaderUtil::ComputeHash(pbData, (uint32_t)pbSize);
    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(type, assetName, hash, pbSize, flags);

    // 确保输出目录存在
    fs::path filePath(outPath);
    fs::path parentDir = filePath.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir))
    {
        std::error_code ec;
        fs::create_directories(parentDir, ec);
    }

    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("AssetContainerWriter: cannot open output %s", outPath.c_str());
        return false;
    }

    if (!AssetFileHeaderUtil::WriteHeader(outFile, header))
    {
        LOG_ERROR("AssetContainerWriter: write header failed for %s", outPath.c_str());
        outFile.close();
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(pbData), (std::streamsize)pbSize);
    outFile.close();

    LOG_INFO("AssetContainerWriter: saved %s (type=%u, %zu bytes)", outPath.c_str(),
             (uint32_t)type, pbSize);
    return true;
}

bool AssetContainerWriter::WriteAssetFile(const std::string& outPath,
                                          AssetType type,
                                          const std::string& assetName,
                                          const std::vector<uint8_t>& pbData,
                                          uint32_t flags)
{
    return WriteAssetFile(outPath, type, assetName,
                          pbData.data(), pbData.size(), flags);
}

NS_ASSETMANAGER_END
