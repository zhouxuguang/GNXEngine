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

float RadicalInverse_VdC(uint32_t bits);

mathutil::Vector2f Hammersley(uint32_t i, uint32_t N);

mathutil::Vector3f ImportanceSampleGGX(const mathutil::Vector2f Xi,
                                       float roughness, const mathutil::Vector3f& N);

float GeometrySchlickGGX(float NdotV, float roughness);

mathutil::Vector2f IntegrateBRDF(float NdotV, float roughness, uint32_t samples);

imagecodec::VImagePtr GenerateBRDFLUT(uint32_t imageSize, uint32_t samples);

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
 * @return 6 个面的预过滤图图像数组，每个面包含完整 mipmap 链（但返回的是 base level，
 *         mipmap 由 GenerateKTXCubemapData 自动生成）
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

NS_ASSETPROCESS_END

#endif // GNX_ENGINE_PBR_BASE_INCLUDE
