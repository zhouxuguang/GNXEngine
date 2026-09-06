//
//  AssetPackager.h
//  GNXEngine
//
//  引擎级离线资产打包封装。
//
//  统一了 pbr_asset_baker 等离线工具所需的资产打包逻辑：
//    - 模型 (gltf/glb/obj/fbx/3ds) -> .meshasset   (MeshMessage + AssetFileHeader)
//    - 2D 贴图 (jpg/png/tga/webp/...) -> .texture    (KTX + TextureMessage + header)
//    - HDR 环境 -> 天空盒 / IBL(irradiance/prefilter/brdfLUT) .texture
//    - 基础几何 (sphere/plane) -> .meshasset
//  底层复用引擎的 AssimpMeshImporter / TextureImporter / PBRBase / EnvHdrProcess
//  与 AssetContainerWriter，打包逻辑在引擎内维护，工具侧只做参数解析。
//

#ifndef GNX_ENGINE_ASSET_PACKAGER_INCLUDE
#define GNX_ENGINE_ASSET_PACKAGER_INCLUDE

#include "AssetProcessDefine.h"
#include <string>
#include <cstdint>

NS_ASSETPROCESS_BEGIN

// 2D 贴图语义类型（决定颜色空间与通道归一化）
enum class TextureMapType : uint32_t
{
    Albedo   = 0,   // 颜色贴图 (sRGB)
    Normal   = 1,   // 法线贴图 (线性)
    Orm      = 2,   // 金属度/粗糙度 (线性)
    AO       = 3,   // 环境光遮蔽 (线性)
    Emissive = 4,   // 自发光 (sRGB)
};

class ASSET_PROCESS_API AssetPackager
{
public:
    AssetPackager() = delete;

    // ==================== 模型打包 ====================

    /**
     * 模型文件 -> .meshasset（引擎统一 assimp 预处理参数 + MeshMessage 编码）
     * @param srcModel 源模型 (gltf/glb/obj/fbx/3ds)
     * @param outMeshAsset 输出 .meshasset 路径
     * @param assetName 资产名（默认取源文件名 stem）
     * @return 成功返回 true
     */
    static bool PackMeshFromFile(const std::string& srcModel,
                                 const std::string& outMeshAsset,
                                 const std::string& assetName = "");

    /**
     * 程序化基础几何 -> .meshasset（sphere / plane）
     */
    static bool PackPrimitiveMesh(const std::string& kind,
                                  const std::string& outMeshAsset);

    // ==================== 2D 贴图打包 ====================

    /**
     * 图片 -> .texture（自动 RGB->RGBA、按 type 打 sRGB/线性标签、BC7 压缩 + mip 链）
     * @param srcImage 源图片
     * @param outTexture 输出 .texture 路径
     * @param type 贴图语义（albedo/emissive=sRGB, normal/orm/ao=线性）
     * @param assetName 资产名（默认取源文件名 stem）
     * @return 成功返回 true
     */
    static bool Pack2DTextureFromFile(const std::string& srcImage,
                                      const std::string& outTexture,
                                      TextureMapType type,
                                      const std::string& assetName = "");

    // ==================== 环境/IBL 打包 ====================

    /**
     * HDR 环境 -> 天空盒 cubemap .texture（BC6H 压缩）
     */
    static bool PackEnvironmentCubemapFromHDR(const std::string& hdrFile,
                                              const std::string& outTexture,
                                              uint32_t faceSize = 512,
                                              const std::string& assetName = "");

    /**
     * HDR 环境 -> IBL 资产组（{prefix}_brdfLUT/irradiance/prefilter.texture）
     * irradiance/prefilter 走 BC6H，brdfLUT 走 RG16F。
     */
    static bool BakeIBLFromHDR(const std::string& hdrFile,
                               const std::string& outPrefix,
                               uint32_t faceSize = 128,
                               uint32_t samples = 256);
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSET_PACKAGER_INCLUDE
