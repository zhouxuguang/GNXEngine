//
//  ModelAssetPackager.h
//  GNXEngine
//
//  引擎级【模型级】离线资产打包封装。
//
//  负责把"一个模型及其材质贴图"打包成模型跟随的资源包：
//    - 模型 (gltf/glb/obj/fbx/3ds) -> .meshasset   (MeshMessage + AssetFileHeader)
//    - 2D 材质贴图 (jpg/png/...)       -> .texture    (KTX + TextureMessage + header)
//    - 程序化基础几何 (sphere/plane)   -> .meshasset
//
//  这些资产属于模型本身（模型换场景也一起走），与场景级环境(天空盒/IBL)解耦，
//  后者由 EnvironmentAssetBaker 负责。
//
//  底层复用引擎的 AssimpMeshImporter / TextureImporter / AssetContainerWriter，
//  打包逻辑在引擎内维护，工具侧只做参数解析。
//

#ifndef GNX_ENGINE_MODEL_ASSET_PACKAGER_INCLUDE
#define GNX_ENGINE_MODEL_ASSET_PACKAGER_INCLUDE

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

class ASSET_PROCESS_API ModelAssetPackager
{
public:
    ModelAssetPackager() = delete;

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
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_MODEL_ASSET_PACKAGER_INCLUDE
