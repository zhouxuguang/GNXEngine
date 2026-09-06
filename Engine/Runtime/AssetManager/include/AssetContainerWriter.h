//
//  AssetContainerWriter.h
//  GNXEngine
//
//  通用资产容器写入器：把"已编码的 nanopb 字节" + AssetFileHeader 写成
//  引擎统一的资产文件（.gnxasset / .meshasset / .texture，布局均为
//  [AssetFileHeader:128B][nanopb payload]）。
//
//  用途：供离线打包工具（tool/pbr_asset_baker 等）与引擎内部模块把
//  MeshMessage / TextureMessage / 其它 pb 载荷写出为带文件头的容器文件，
//  调用方无需重复 ComputeHash / CreateHeader / WriteHeader 逻辑。
//

#ifndef GNX_ENGINE_ASSET_CONTAINER_WRITER_INCLUDE
#define GNX_ENGINE_ASSET_CONTAINER_WRITER_INCLUDE

#include "AssetDefine.h"
#include "AssetType.h"
#include "AssetFileHeader.h"
#include <string>
#include <vector>
#include <cstdint>

NS_ASSETMANAGER_BEGIN

class ASSET_MANAGER_API AssetContainerWriter
{
public:
    AssetContainerWriter() = delete;

    /**
     * 把 pb 载荷写成资产容器文件（[AssetFileHeader][pb]）
     * @param outPath 输出文件路径（可含子目录，会自动创建）
     * @param type 资产类型（Mesh/Texture/Shader...）
     * @param assetName 资产名（写入 header.name，便于调试，可传文件名）
     * @param pbData 已编码的 nanopb 字节
     * @param pbSize pb 字节数
     * @param flags AssetFileFlags（如 COMPRESSED）
     * @return 成功返回 true
     */
    static bool WriteAssetFile(const std::string& outPath,
                               AssetType type,
                               const std::string& assetName,
                               const uint8_t* pbData,
                               size_t pbSize,
                               uint32_t flags = AssetFileFlags::NONE);

    /**
     * std::vector 便捷重载
     */
    static bool WriteAssetFile(const std::string& outPath,
                               AssetType type,
                               const std::string& assetName,
                               const std::vector<uint8_t>& pbData,
                               uint32_t flags = AssetFileFlags::NONE);
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_ASSET_CONTAINER_WRITER_INCLUDE
