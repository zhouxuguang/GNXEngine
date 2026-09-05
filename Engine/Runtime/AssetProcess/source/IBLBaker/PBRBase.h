//
//  PBRBase.h
//  GNXEngine
//
//  Created by zhouxuguang on 2026/2/12.
//

#ifndef GNX_ENGINE_PBR_BASE_INCLUDE
#define GNX_ENGINE_PBR_BASE_INCLUDE

#include "Runtime/AssetProcess/include/AssetProcessDefine.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include <vector>

NS_ASSETPROCESS_BEGIN

ASSET_PROCESS_API float RadicalInverse_VdC(uint32_t bits);

ASSET_PROCESS_API mathutil::Vector2f Hammersley(uint32_t i, uint32_t N);

ASSET_PROCESS_API mathutil::Vector3f ImportanceSampleGGX(const mathutil::Vector2f Xi,
                                       float roughness, const mathutil::Vector3f& N);

ASSET_PROCESS_API float GeometrySchlickGGX(float NdotV, float roughness);

ASSET_PROCESS_API mathutil::Vector2f IntegrateBRDF(float NdotV, float roughness, uint32_t samples);

ASSET_PROCESS_API imagecodec::VImagePtr GenerateBRDFLUT(uint32_t imageSize, uint32_t samples);

ASSET_PROCESS_API void GenerateBRDFLUT_Texture(const std::string& fileName, uint32_t imageSize, uint32_t samples);

/**
 * @brief 从 Cubemap 6 面图像生成漫反射辐照度图（Diffuse Irradiance Map）
 * 对每个 texel 方向做半球余弦加权积分，生成低频漫反射环境光照
 * @param faces 6 个 Cubemap 面图像（+X, -X, +Y, -Y, +Z, -Z），须为 RGB32Float 格式
 * @param imageSize 输出辐照度图的面尺寸（默认 32，因为漫反射是低频信号）
 * @param samples 每个 texel 的采样数（默认 512）
 * @return 6 个面的辐照度图图像（RGB32Float 格式）
 */
ASSET_PROCESS_API std::vector<imagecodec::VImagePtr> GenerateIrradianceMap(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize = 32, uint32_t samples = 512);

/**
 * @brief 从 Cubemap 6 面图像生成预过滤环境贴图（Prefiltered Environment Map）
 * 每个 mip level 对应不同的粗糙度，使用 GGX 重要性采样进行卷积
 * @param faces 6 个 Cubemap 面图像（+X, -X, +Y, -Y, +Z, -Z），须为 RGB32Float 格式
 * @param imageSize 输出预过滤图的基础面尺寸（默认 128）
 * @param samples 每个 texel 的采样数（默认 256）
 * @return 6 个面的预过滤图图像数组（RGB32Float 格式）
 */
ASSET_PROCESS_API std::vector<imagecodec::VImagePtr> GeneratePrefilteredEnvMap(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize = 128, uint32_t samples = 256);

/**
 * @brief 生成辐照度图并保存为 KTX Cubemap 文件
 */
ASSET_PROCESS_API void GenerateIrradianceMap_Texture(
    const std::string& fileName,
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize = 32, uint32_t samples = 512);

/**
 * @brief 生成预过滤环境贴图并保存为 KTX Cubemap 文件
 */
ASSET_PROCESS_API void GeneratePrefilteredEnvMap_Texture(
    const std::string& fileName,
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize = 128, uint32_t samples = 256);

/**
 * @brief 生成逐 mip 物理正确的预过滤环境贴图，编码为 KTX Cubemap 数据
 * 每个 mip level 对应一个 roughness（GGX 重要性采样卷积），而非简单降采样。
 * 结果可直接包装为 .texture 资产供运行时 IBL specular 采样。
 * @param faces 6 个源 Cubemap 面（RGB32Float）
 * @param baseSize 基础面尺寸（默认 128）
 * @param samples 每 texel 采样数（默认 256）
 * @param outData 输出的 KTX1 Cubemap 字节
 * @return 成功返回 true
 */
ASSET_PROCESS_API bool GeneratePrefilteredEnvMapMipChain_Data(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t baseSize, uint32_t samples,
    std::vector<uint8_t>& outData);

/**
 * @brief 将 6 面 RGB32Float cubemap 编码为 RGBA32F KTX Cubemap（非压缩，单 mip）
 * 用于环境 cubemap / 漫反射辐照度贴图资产。
 * 说明：BC6H 压缩在部分后端映射缺失，这里统一输出 RGBA32F 以保兼容。
 * @param faces 6 面 RGB32Float 图像
 * @param outData 输出的 KTX1 Cubemap 字节（RGBA32F）
 * @return 成功返回 true
 */
ASSET_PROCESS_API bool GenerateRGBA32FCubemap_Data(
    const std::vector<imagecodec::VImagePtr>& faces,
    std::vector<uint8_t>& outData);

/**
 * @brief 将 2D RGBA8/sRGB8_ALPHA8 图像编码为未压缩 KTX 纹理（带完整 mip 链）
 * 用于部分后端（如 Metal 映射缺失 BC7/BC6H）时以未压缩格式输出资产。
 * @param image 源图像（RGBA8 或 SRGB8_ALPHA8；其它格式会被转为 RGBA8）
 * @param srgb 是否标记为 sRGB（颜色贴图 true；normal/orm/ao false）
 * @param outData 输出的 KTX1 字节（含 mip 链）
 * @return 成功返回 true
 */
ASSET_PROCESS_API bool GenerateUncompressed2DKTX_Data(
    imagecodec::VImagePtr image, bool srgb,
    std::vector<uint8_t>& outData);

NS_ASSETPROCESS_END

#endif // GNX_ENGINE_PBR_BASE_INCLUDE
