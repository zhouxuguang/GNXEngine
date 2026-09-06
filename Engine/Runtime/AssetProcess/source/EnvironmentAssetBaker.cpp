//
//  EnvironmentAssetBaker.cpp
//  GNXEngine
//
//  引擎级【场景级】环境资产离线烘焙实现。
//  天空盒 cubemap / IBL(irradiance/prefilter/brdfLUT) 打包逻辑（原 AssetPackager 中环境部分）。
//

#include "EnvironmentAssetBaker.h"
#include "AssetPackHelpers.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include "Runtime/AssetProcess/source/TextureProcess/EnvHdrProcess.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/VImage.h"

#include <vector>

using namespace imagecodec;

NS_ASSETPROCESS_BEGIN

namespace
{
// HDR -> 6 面 cubemap（RGB32Float faces）
std::vector<VImagePtr> LoadEnvironmentFaces(const std::string& hdrFile)
{
    VImage image;
    if (!ImageDecoder::DecodeFile(hdrFile.c_str(), &image))
    {
        LOG_ERROR("EnvironmentAssetBaker: failed to decode HDR: %s", hdrFile.c_str());
        return {};
    }
    LOG_INFO("EnvironmentAssetBaker: HDR loaded: %ux%u format=%d", image.GetWidth(), image.GetHeight(), image.GetFormat());

    VImagePtr cross = ConvertEquirectangularMapToVerticalCross(&image);
    if (!cross)
    {
        LOG_ERROR("EnvironmentAssetBaker: equirect -> vertical cross failed");
        return {};
    }

    std::vector<VImagePtr> faces = ConvertVerticalCrossToCubeMapFaces(cross.get());
    if (faces.size() != 6)
    {
        LOG_ERROR("EnvironmentAssetBaker: failed to split cubemap into 6 faces");
        return {};
    }

    LOG_INFO("EnvironmentAssetBaker: environment faces: %ux%u x6", faces[0]->GetWidth(), faces[0]->GetHeight());
    return faces;
}
} // namespace

bool EnvironmentAssetBaker::PackEnvironmentCubemapFromHDR(const std::string& hdrFile,
                                                          const std::string& outTexture,
                                                          uint32_t faceSize,
                                                          const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? AssetPackHelpers::GetStemName(outTexture) : assetNameIn;

    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile);
    if (faces.size() != 6)
    {
        return false;
    }
    (void)faceSize;   // 面尺寸由源 HDR 决定（保持与既有资产一致）

    // RGB32Float -> BC6H_UFLOAT 压缩（TextureImporter 按 VImage 格式自动选 BC6H）
    TextureImporter importer;
    TextureImportSettings s;
    s.mipmapMode = MipmapMode::None;
    std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(faces, s);
    if (ktx.empty())
    {
        LOG_ERROR("EnvironmentAssetBaker: cubemap KTX encode failed for %s", hdrFile.c_str());
        return false;
    }

    return AssetPackHelpers::WriteTextureContainer(outTexture, ktx, assetName,
                                                   AssetManager::AssetFileFlags::COMPRESSED);
}

bool EnvironmentAssetBaker::BakeIBLFromHDR(const std::string& hdrFile,
                                           const std::string& outPrefix,
                                           uint32_t faceSize,
                                           uint32_t samples)
{
    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile);
    if (faces.size() != 6)
    {
        return false;
    }

    // --- BRDF LUT（RG16Float，非压缩） ---
    {
        std::string outPath = outPrefix + "_brdfLUT.texture";
        VImagePtr lut = GenerateBRDFLUT(512, 1024);
        if (!lut)
        {
            LOG_ERROR("EnvironmentAssetBaker: BRDF LUT generation failed");
            return false;
        }
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        s.enableCompression = false;
        std::vector<uint8_t> ktx = importer.GenerateKTXData(lut, s);
        if (ktx.empty())
        {
            LOG_ERROR("EnvironmentAssetBaker: BRDF LUT KTX encode failed");
            return false;
        }
        if (!AssetPackHelpers::WriteTextureContainer(outPath, ktx, "brdfLUT"))
        {
            return false;
        }
    }

    // --- Irradiance（漫反射辐照度，32x32，BC6H） ---
    {
        std::string outPath = outPrefix + "_irradiance.texture";
        std::vector<VImagePtr> irrFaces = GenerateIrradianceMap(faces, 32, samples);
        if (irrFaces.empty())
        {
            LOG_ERROR("EnvironmentAssetBaker: irradiance generation failed");
            return false;
        }
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(irrFaces, s);
        if (ktx.empty())
        {
            LOG_ERROR("EnvironmentAssetBaker: irradiance KTX encode failed");
            return false;
        }
        if (!AssetPackHelpers::WriteTextureContainer(outPath, ktx, "env_irradiance",
                                                     AssetManager::AssetFileFlags::COMPRESSED))
        {
            return false;
        }
    }

    // --- Prefiltered（高光预过滤，BC6H，逐 mip GGX 卷积） ---
    {
        std::string outPath = outPrefix + "_prefilter.texture";
        std::vector<uint8_t> ktx;
        if (!GeneratePrefilteredEnvMapMipChain_Data(faces, faceSize, samples, ktx))
        {
            LOG_ERROR("EnvironmentAssetBaker: prefiltered env mip-chain generation failed");
            return false;
        }
        if (!AssetPackHelpers::WriteTextureContainer(outPath, ktx, "env_prefilter",
                                                     AssetManager::AssetFileFlags::COMPRESSED))
        {
            return false;
        }
    }

    return true;
}

NS_ASSETPROCESS_END
