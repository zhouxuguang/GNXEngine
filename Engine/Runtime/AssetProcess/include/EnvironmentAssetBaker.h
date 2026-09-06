//
//  EnvironmentAssetBaker.h
//  GNXEngine
//
//  引擎级【场景级】环境资产离线烘焙封装。
//
//  负责把一张 HDR 环境图烘焙成"场景环境"产物（一个场景可配多套，多个模型共享）：
//    - 天空盒 cubemap            -> .texture (BC6H)
//    - IBL 组                    -> {prefix}_brdfLUT / _irradiance / _prefilter.texture
//        irradiance/prefilter = BC6H_UFLOAT, brdfLUT = RG16F
//
//  与模型级资源（meshasset + 材质贴图，ModelAssetPackager 负责）解耦：环境资产
//  属于场景/光照预设，不跟任何单个模型走。
//
//  底层复用引擎的 EnvHdrProcess / PBRBase / TextureImporter / AssetContainerWriter。
//

#ifndef GNX_ENGINE_ENVIRONMENT_ASSET_BAKER_INCLUDE
#define GNX_ENGINE_ENVIRONMENT_ASSET_BAKER_INCLUDE

#include "AssetProcessDefine.h"
#include <string>
#include <cstdint>

NS_ASSETPROCESS_BEGIN

class ASSET_PROCESS_API EnvironmentAssetBaker
{
public:
    EnvironmentAssetBaker() = delete;

    /**
     * HDR 环境 -> 天空盒 cubemap .texture（BC6H 压缩）
     * @param hdrFile 源 HDR 环境图
     * @param outTexture 输出 .texture 路径
     * @param faceSize 目标面尺寸（当前由源 HDR 决定，保留参数以兼容）
     * @param assetName 资产名（默认取输出文件名 stem）
     * @return 成功返回 true
     */
    static bool PackEnvironmentCubemapFromHDR(const std::string& hdrFile,
                                              const std::string& outTexture,
                                              uint32_t faceSize = 512,
                                              const std::string& assetName = "");

    /**
     * HDR 环境 -> IBL 资产组（{prefix}_brdfLUT/irradiance/prefilter.texture）
     * irradiance/prefilter 走 BC6H，brdfLUT 走 RG16F。
     * @param hdrFile 源 HDR 环境图
     * @param outPrefix 输出前缀（如 data_asset/pbr/env/1 -> 生成 1_brdfLUT.texture 等）
     * @param faceSize 预过滤基础面尺寸
     * @param samples 每 texel 采样数
     * @return 成功返回 true
     */
    static bool BakeIBLFromHDR(const std::string& hdrFile,
                               const std::string& outPrefix,
                               uint32_t faceSize = 128,
                               uint32_t samples = 256);
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ENVIRONMENT_ASSET_BAKER_INCLUDE
