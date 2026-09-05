//
//  PBRBase.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2026/2/12.
//

#include "PBRBase.h"
#include "Runtime/MathUtil/include/HalfFloat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/ImageCodec/include/ImageUtil.h"
#include "TextureImporter.h"
#include <ktx.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>

NS_ASSETPROCESS_BEGIN

const float PI = 3.14159265358979323846264338327950288;

// ==================== Cubemap 采样辅助函数 ====================

/**
 * 根据方向向量确定 cubemap 面索引和 UV 坐标
 * 面顺序: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
 */
static void DirectionToFaceUV(const mathutil::Vector3f& dir, uint32_t& face, float& u, float& v)
{
    const float absX = std::abs(dir.x);
    const float absY = std::abs(dir.y);
    const float absZ = std::abs(dir.z);

    float sc, tc, ma;

    if (absX >= absY && absX >= absZ)
    {
        if (dir.x > 0) { face = 0; sc = -dir.z; tc = -dir.y; ma = absX; } // +X
        else           { face = 1; sc = dir.z;  tc = -dir.y; ma = absX; } // -X
    }
    else if (absY >= absX && absY >= absZ)
    {
        if (dir.y > 0) { face = 2; sc = dir.x;  tc = dir.z;  ma = absY; } // +Y
        else           { face = 3; sc = dir.x;  tc = -dir.z; ma = absY; } // -Y
    }
    else
    {
        if (dir.z > 0) { face = 4; sc = dir.x;  tc = -dir.y; ma = absZ; } // +Z
        else           { face = 5; sc = -dir.x; tc = -dir.y; ma = absZ; } // -Z
    }

    u = 0.5f * (sc / ma + 1.0f);
    v = 0.5f * (tc / ma + 1.0f);
}

/**
 * 从 cubemap 6 面图像中采样颜色（双线性插值）
 * 输入图像须为 RGB32Float 格式
 */
static mathutil::Vector3f SampleCubemap(const std::vector<imagecodec::VImagePtr>& faces,
                                         const mathutil::Vector3f& dir)
{
    uint32_t face = 0;
    float u = 0, v = 0;
    DirectionToFaceUV(dir, face, u, v);

    const imagecodec::VImage* img = faces[face].get();
    uint32_t w = img->GetWidth();
    uint32_t h = img->GetHeight();

    // 映射到像素坐标
    float px = u * w - 0.5f;
    float py = v * h - 0.5f;

    int x0 = std::max(0, std::min((int)std::floor(px), (int)w - 1));
    int y0 = std::max(0, std::min((int)std::floor(py), (int)h - 1));
    int x1 = std::min(x0 + 1, (int)w - 1);
    int y1 = std::min(y0 + 1, (int)h - 1);

    float fx = px - std::floor(px);
    float fy = py - std::floor(py);

    const float* pData = (const float*)img->GetImageData();

    auto sample = [&](int x, int y) -> mathutil::Vector3f {
        uint32_t offset = (y * w + x) * 3;
        return mathutil::Vector3f(pData[offset], pData[offset + 1], pData[offset + 2]);
    };

    mathutil::Vector3f c00 = sample(x0, y0);
    mathutil::Vector3f c10 = sample(x1, y0);
    mathutil::Vector3f c01 = sample(x0, y1);
    mathutil::Vector3f c11 = sample(x1, y1);

    return c00 * (1 - fx) * (1 - fy) + c10 * fx * (1 - fy) +
           c01 * (1 - fx) * fy + c11 * fx * fy;
}

/**
 * 构建局部坐标系（给定向量 N 为 z 轴）
 */
static void BuildOrthonormalBasis(const mathutil::Vector3f& n,
                                   mathutil::Vector3f& tangent,
                                   mathutil::Vector3f& bitangent)
{
    mathutil::Vector3f up = (std::abs(n.z) < 0.999f)
        ? mathutil::Vector3f(0.0f, 0.0f, 1.0f)
        : mathutil::Vector3f(1.0f, 0.0f, 0.0f);
    tangent = mathutil::Vector3f::CrossProduct(up, n).Normalize();
    bitangent = mathutil::Vector3f::CrossProduct(n, tangent);
}

// ==================== 原有 BRDF LUT 函数 ====================

float RadicalInverse_VdC(uint32_t bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

mathutil::Vector2f Hammersley(uint32_t i, uint32_t N)
{
    return mathutil::Vector2f(float(i) / float(N), RadicalInverse_VdC(i));
}

mathutil::Vector3f ImportanceSampleGGX(const mathutil::Vector2f Xi,
                              float roughness, const mathutil::Vector3f& N)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    mathutil::Vector3f H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    mathutil::Vector3f up = abs(N.z) < 0.999 ?
        mathutil::Vector3f(0.0, 0.0, 1.0) : mathutil::Vector3f(1.0, 0.0, 0.0);
    mathutil::Vector3f tangent = mathutil::Vector3f::CrossProduct(up, N).Normalize();
    mathutil::Vector3f bitangent = mathutil::Vector3f::CrossProduct(N, tangent);

    mathutil::Vector3f sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return sampleVec.Normalize();
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float roughness, float NoV, float NoL)
{
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

mathutil::Vector2f IntegrateBRDF(float NdotV, float roughness, uint32_t samples)
{
    mathutil::Vector3f V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    mathutil::Vector3f N = mathutil::Vector3f(0.0, 0.0, 1.0);

    for (uint32_t i = 0u; i < samples; ++i)
    {
        mathutil::Vector2f Xi = Hammersley(i, samples);
        mathutil::Vector3f H = ImportanceSampleGGX(Xi, roughness, N);
        mathutil::Vector3f L = (2.0f * V.DotProduct(H) * H - V).Normalize();

        float NoL = std::max(L.z, 0.0f);
        float NoH = std::max(H.z, 0.0f);
        float VoH = std::max(V.DotProduct(H), 0.0f);
        float NoV = std::max(N.DotProduct(V), 0.0f);

        if (NoL > 0.0)
        {
            float G = GeometrySmith(roughness, NoV, NoL);

            float G_Vis = (G * VoH) / (NoH * NoV);
            float Fc = pow(1.0 - VoH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return mathutil::Vector2f(A / float(samples), B / float(samples));
}

imagecodec::VImagePtr GenerateBRDFLUT(uint32_t imageSize, uint32_t samples)
{
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    image->SetImageInfo(imagecodec::FORMAT_RG16Float,
                        imageSize, imageSize);
    image->AllocPixels();
    
    uint16_t* pImageData = (uint16_t*)image->GetImageData();
    
    for (int x = 0; x < imageSize; x++)
    {
        for (int y = 0; y < imageSize; y++)
        {
            // The shader samples the LUT as (NdotV, roughness).
            float NoV = (x + 0.5f) * (1.0f / imageSize);
            float roughness = (y + 0.5f) * (1.0f / imageSize);
            
            uint32_t offset = (y * imageSize + x) * 2;
            
            mathutil::Vector2f brdfValue = IntegrateBRDF(NoV, roughness, samples);
            pImageData[offset] = float_to_half(brdfValue.x);
            pImageData[offset + 1] = float_to_half(brdfValue.y);
            
        }
    }
    return image;
}

void GenerateBRDFLUT_Texture(const std::string& fileName, uint32_t imageSize, uint32_t samples)
{
    imagecodec::VImagePtr image = GenerateBRDFLUT(imageSize, samples);
    TextureImporter textureImporter;
    TextureImportSettings textureImportSettings;
    textureImportSettings.mipmapMode = MipmapMode::None;
    std::vector<uint8_t> ktxData = textureImporter.GenerateKTXData(image, textureImportSettings);
    
    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open texture file for writing: %s", fileName.c_str());
        return;
    }

    // 写入 ktx 数据
    outFile.write(reinterpret_cast<const char*>(ktxData.data()), ktxData.size());
    outFile.close();
}

// ==================== Irradiance Map 生成 ====================

std::vector<imagecodec::VImagePtr> GenerateIrradianceMap(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize, uint32_t samples)
{
    if (faces.size() != 6)
    {
        LOG_ERROR("GenerateIrradianceMap requires exactly 6 faces");
        return {};
    }

    LOG_INFO("Generating irradiance map (%ux%u, %d samples)...", imageSize, imageSize, samples);

    std::vector<imagecodec::VImagePtr> resultFaces;

    for (uint32_t face = 0; face < 6; ++face)
    {
        imagecodec::VImagePtr outImage = std::make_shared<imagecodec::VImage>();
        outImage->SetImageInfo(imagecodec::FORMAT_RGB32Float, imageSize, imageSize);
        outImage->AllocPixels();

        float* pOutData = (float*)outImage->GetImageData();

        for (uint32_t y = 0; y < imageSize; ++y)
        {
            for (uint32_t x = 0; x < imageSize; ++x)
            {
                // 计算当前 texel 对应的方向向量
                float u = (x + 0.5f) / imageSize;
                float v = (y + 0.5f) / imageSize;

                // 从 UV 和面索引反推方向
                float su = 2.0f * u - 1.0f;
                float sv = 2.0f * v - 1.0f;

                mathutil::Vector3f N;
                switch (face)
                {
                case 0: N = mathutil::Vector3f(1.0f, -sv, -su).Normalize();  break; // +X
                case 1: N = mathutil::Vector3f(-1.0f, -sv, su).Normalize();  break; // -X
                case 2: N = mathutil::Vector3f(su, 1.0f, sv).Normalize();    break; // +Y
                case 3: N = mathutil::Vector3f(su, -1.0f, -sv).Normalize();  break; // -Y
                case 4: N = mathutil::Vector3f(su, -sv, 1.0f).Normalize();   break; // +Z
                case 5: N = mathutil::Vector3f(-su, -sv, -1.0f).Normalize(); break; // -Z
                }

                // 构建局部坐标系
                mathutil::Vector3f tangent, bitangent;
                BuildOrthonormalBasis(N, tangent, bitangent);

                // 半球余弦加权积分（irradiance）
                mathutil::Vector3f irradiance(0.0f, 0.0f, 0.0f);

                for (uint32_t i = 0; i < samples; ++i)
                {
                    // 余弦加权半球采样（Malley's method）
                    // rand1 = 方位角均匀，rand2 = Hammersley 序列
                    float rand1 = (float)i / samples + 0.5f / samples;
                    float rand2 = RadicalInverse_VdC(i);

                    // 余弦加权半球采样：pdf(θ) = cosθ/π
                    // 方向分布由 cosθ = sqrt(1-rand2) 保证（圆盘均匀投影）
                    float phi = 2.0f * PI * rand1;
                    float cosTheta = std::sqrt(1.0f - rand2);
                    float sinTheta = std::sqrt(rand2);

                    // 局部坐标下的方向
                    float lx = cos(phi) * sinTheta;
                    float ly = sin(phi) * sinTheta;
                    float lz = cosTheta;

                    // 变换到世界坐标
                    mathutil::Vector3f sampleDir = tangent * lx + bitangent * ly + N * lz;

                    // 从环境贴图采样
                    mathutil::Vector3f color = SampleCubemap(faces, sampleDir);

                    // 重要采样估计 E/π = ∫L(ω)cosθdω / π：
                    //   用 pdf=cosθ/π 的余弦加权采样，E = (1/N)Σ L/pdf = (π/N)Σ L，
                    //   而漫反射 IBL 需要 E/π，故 irradiance += L（cosθ 与 pdf 已抵消），
                    //   最后除以 samples。
                    // ⚠️ 旧实现误乘 cosθ 再乘 π/N，导致 irradiance 偏大约 2.09 倍 → 画面发白。
                    irradiance = irradiance + color;
                }

                irradiance = irradiance * (1.0f / (float)samples);

                uint32_t pixelOffset = (y * imageSize + x) * 3;
                pOutData[pixelOffset + 0] = irradiance.x;
                pOutData[pixelOffset + 1] = irradiance.y;
                pOutData[pixelOffset + 2] = irradiance.z;
            }
        }

        resultFaces.push_back(outImage);
        LOG_INFO("  Face %d done", face);
    }

    LOG_INFO("Irradiance map generation complete");
    return resultFaces;
}

// ==================== Prefiltered Environment Map 生成 ====================

std::vector<imagecodec::VImagePtr> GeneratePrefilteredEnvMap(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize, uint32_t samples)
{
    if (faces.size() != 6)
    {
        LOG_ERROR("GeneratePrefilteredEnvMap requires exactly 6 faces");
        return {};
    }

    LOG_INFO("Generating prefiltered environment map (%ux%u, %d samples)...", imageSize, imageSize, samples);

    std::vector<imagecodec::VImagePtr> resultFaces;

    for (uint32_t face = 0; face < 6; ++face)
    {
        imagecodec::VImagePtr outImage = std::make_shared<imagecodec::VImage>();
        outImage->SetImageInfo(imagecodec::FORMAT_RGB32Float, imageSize, imageSize);
        outImage->AllocPixels();

        float* pOutData = (float*)outImage->GetImageData();

        for (uint32_t y = 0; y < imageSize; ++y)
        {
            for (uint32_t x = 0; x < imageSize; ++x)
            {
                // 计算当前 texel 对应的方向向量
                float u = (x + 0.5f) / imageSize;
                float v = (y + 0.5f) / imageSize;

                float su = 2.0f * u - 1.0f;
                float sv = 2.0f * v - 1.0f;

                mathutil::Vector3f N;
                switch (face)
                {
                case 0: N = mathutil::Vector3f(1.0f, -sv, -su).Normalize();  break;
                case 1: N = mathutil::Vector3f(-1.0f, -sv, su).Normalize();  break;
                case 2: N = mathutil::Vector3f(su, 1.0f, sv).Normalize();    break;
                case 3: N = mathutil::Vector3f(su, -1.0f, -sv).Normalize();  break;
                case 4: N = mathutil::Vector3f(su, -sv, 1.0f).Normalize();   break;
                case 5: N = mathutil::Vector3f(-su, -sv, -1.0f).Normalize(); break;
                }

                // 根据像素坐标推导 roughness（较低 mip level = 更粗糙）
                // 这里生成的是 base level，roughness=0 时的预过滤结果
                // mipmap 链会在 KTX 写入时自动生成，不同 mip level 对应不同 roughness
                // 但为了简单起见，这里我们只生成 roughness=0 的 base level
                // 更完整的实现需要为每个 mip level 分别生成
                float roughness = 0.0f;

                // GGX 重要性采样卷积
                mathutil::Vector3f prefilteredColor(0.0f, 0.0f, 0.0f);
                float totalWeight = 0.0f;

                mathutil::Vector3f tangent, bitangent;
                BuildOrthonormalBasis(N, tangent, bitangent);

                for (uint32_t i = 0; i < samples; ++i)
                {
                    mathutil::Vector2f Xi = Hammersley(i, samples);
                    mathutil::Vector3f H = ImportanceSampleGGX(Xi, roughness, N);

                    // 计算入射方向 L = 2 * (V·H) * H - V, 其中 V = N
                    float NoH = std::max(N.DotProduct(H), 0.0f);
                    mathutil::Vector3f L = (2.0f * NoH * H - N).Normalize();

                    float NoL = std::max(N.DotProduct(L), 0.0f);
                    if (NoL > 0.0f)
                    {
                        prefilteredColor = prefilteredColor + SampleCubemap(faces, L) * NoL;
                        totalWeight += NoL;
                    }
                }

                if (totalWeight > 0.0f)
                {
                    prefilteredColor = prefilteredColor * (1.0f / totalWeight);
                }

                uint32_t pixelOffset = (y * imageSize + x) * 3;
                pOutData[pixelOffset + 0] = prefilteredColor.x;
                pOutData[pixelOffset + 1] = prefilteredColor.y;
                pOutData[pixelOffset + 2] = prefilteredColor.z;
            }
        }

        resultFaces.push_back(outImage);
        LOG_INFO("  Face %d done", face);
    }

    LOG_INFO("Prefiltered environment map (base level) generation complete");
    return resultFaces;
}

// ==================== 生成各 mip level 的预过滤环境贴图 ====================

/**
 * 为指定 roughness 生成一个 cubemap 面的预过滤结果
 */
static imagecodec::VImagePtr PrefilterEnvMapFace(
    const std::vector<imagecodec::VImagePtr>& srcFaces,
    float roughness, uint32_t faceIdx, uint32_t faceSize, uint32_t samples)
{
    imagecodec::VImagePtr outImage = std::make_shared<imagecodec::VImage>();
    outImage->SetImageInfo(imagecodec::FORMAT_RGB32Float, faceSize, faceSize);
    outImage->AllocPixels();

    float* pOutData = (float*)outImage->GetImageData();

    for (uint32_t y = 0; y < faceSize; ++y)
    {
        for (uint32_t x = 0; x < faceSize; ++x)
        {
            float u = (x + 0.5f) / faceSize;
            float v = (y + 0.5f) / faceSize;
            float su = 2.0f * u - 1.0f;
            float sv = 2.0f * v - 1.0f;

            mathutil::Vector3f N;
            switch (faceIdx)
            {
            case 0: N = mathutil::Vector3f(1.0f, -sv, -su).Normalize();  break;
            case 1: N = mathutil::Vector3f(-1.0f, -sv, su).Normalize();  break;
            case 2: N = mathutil::Vector3f(su, 1.0f, sv).Normalize();    break;
            case 3: N = mathutil::Vector3f(su, -1.0f, -sv).Normalize();  break;
            case 4: N = mathutil::Vector3f(su, -sv, 1.0f).Normalize();   break;
            case 5: N = mathutil::Vector3f(-su, -sv, -1.0f).Normalize(); break;
            }

            mathutil::Vector3f prefilteredColor(0.0f, 0.0f, 0.0f);
            float totalWeight = 0.0f;

            mathutil::Vector3f tangent, bitangent;
            BuildOrthonormalBasis(N, tangent, bitangent);

            for (uint32_t i = 0; i < samples; ++i)
            {
                mathutil::Vector2f Xi = Hammersley(i, samples);
                mathutil::Vector3f H = ImportanceSampleGGX(Xi, roughness, N);

                float NoH = std::max(N.DotProduct(H), 0.0f);
                mathutil::Vector3f L = (2.0f * NoH * H - N).Normalize();

                float NoL = std::max(N.DotProduct(L), 0.0f);
                if (NoL > 0.0f)
                {
                    prefilteredColor = prefilteredColor + SampleCubemap(srcFaces, L) * NoL;
                    totalWeight += NoL;
                }
            }

            if (totalWeight > 0.0f)
            {
                prefilteredColor = prefilteredColor * (1.0f / totalWeight);
            }

            uint32_t pixelOffset = (y * faceSize + x) * 3;
            pOutData[pixelOffset + 0] = prefilteredColor.x;
            pOutData[pixelOffset + 1] = prefilteredColor.y;
            pOutData[pixelOffset + 2] = prefilteredColor.z;
        }
    }

    return outImage;
}

// ==================== 保存为 KTX 文件的便捷函数 ====================

void GenerateIrradianceMap_Texture(
    const std::string& fileName,
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize, uint32_t samples)
{
    std::vector<imagecodec::VImagePtr> irradianceFaces = GenerateIrradianceMap(faces, imageSize, samples);
    if (irradianceFaces.empty()) return;

    TextureImporter textureImporter;
    TextureImportSettings textureImportSettings;
    textureImportSettings.mipmapMode = MipmapMode::None; // 漫反射辐照图不需要 mipmap
    std::vector<uint8_t> ktxData = textureImporter.GenerateKTXCubemapData(irradianceFaces, textureImportSettings);

    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open irradiance map file for writing: %s", fileName.c_str());
        return;
    }

    outFile.write(reinterpret_cast<const char*>(ktxData.data()), ktxData.size());
    outFile.close();

    LOG_INFO("Irradiance map saved to: %s (%zu bytes)", fileName.c_str(), ktxData.size());
}

void GeneratePrefilteredEnvMap_Texture(
    const std::string& fileName,
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t imageSize, uint32_t samples)
{
    // 每个 mip 都必须使用对应 roughness 的 GGX 卷积，不能用普通降采样替代。
    std::vector<uint8_t> ktxData;
    if (!GeneratePrefilteredEnvMapMipChain_Data(faces, imageSize, samples, ktxData))
    {
        return;
    }

    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open prefiltered env map file for writing: %s", fileName.c_str());
        return;
    }

    outFile.write(reinterpret_cast<const char*>(ktxData.data()), ktxData.size());
    outFile.close();

    LOG_INFO("Prefiltered environment map saved to: %s (%zu bytes)", fileName.c_str(), ktxData.size());
}

// ==================== 逐 mip 物理正确的预过滤环境贴图 ====================

/**
 * 把 6 面 RGB32Float 图像转换为单张 RGBA32Float 数据（w 填充 1.0）
 */
static void ConvertRGBFacesToRGBA32F(const std::vector<imagecodec::VImagePtr>& rgbFaces,
                                     uint32_t faceSize,
                                     std::vector<uint8_t>& outData)
{
    // KTX1 cubemap 的 level 数据布局：连续 6 面，每面 faceSize*faceSize*4 floats
    const uint32_t kBytesPerPixel = 4 * sizeof(float);   // RGBA32F = 16 bytes
    const uint32_t faceBytes = faceSize * faceSize * kBytesPerPixel;
    outData.resize(faceBytes * 6);

    for (uint32_t f = 0; f < 6; ++f)
    {
        const imagecodec::VImage* src = rgbFaces[f].get();
        const float* pSrc = (const float*)src->GetImageData();   // RGB32Float (3 floats)
        float* pDst = (float*)(outData.data() + (size_t)f * faceBytes);

        uint32_t pixelCount = faceSize * faceSize;
        for (uint32_t i = 0; i < pixelCount; ++i)
        {
            pDst[i * 4 + 0] = pSrc[i * 3 + 0];
            pDst[i * 4 + 1] = pSrc[i * 3 + 1];
            pDst[i * 4 + 2] = pSrc[i * 3 + 2];
            pDst[i * 4 + 3] = 1.0f;
        }
    }
}

bool GeneratePrefilteredEnvMapMipChain_Data(
    const std::vector<imagecodec::VImagePtr>& faces,
    uint32_t baseSize, uint32_t samples,
    std::vector<uint8_t>& outData)
{
    if (faces.size() != 6)
    {
        LOG_ERROR("GeneratePrefilteredEnvMapMipChain requires exactly 6 faces");
        return false;
    }

    // KTX1 常量（与 TextureImporter.cpp 保持一致）
    const uint32_t kGL_RGBA32F = 0x8814;
    const uint32_t kVK_R32G32B32A32_SFLOAT = 109;

    uint32_t numMips = imagecodec::ImageUtil::CalcNumMipLevels(baseSize, baseSize);
    LOG_INFO("Generating prefiltered env map mip-chain: base %ux%u, %u mips, %u samples/texel",
             baseSize, baseSize, numMips, samples);

    // 创建 KTX1 Cubemap 纹理对象（RGBA32F 非压缩）
    ktxTextureCreateInfo createInfo = {};
    createInfo.glInternalformat = kGL_RGBA32F;
    createInfo.vkFormat = kVK_R32G32B32A32_SFLOAT;
    createInfo.baseWidth = baseSize;
    createInfo.baseHeight = baseSize;
    createInfo.baseDepth = 1u;
    createInfo.numDimensions = 2u;
    createInfo.numLevels = numMips;
    createInfo.numLayers = 1u;
    createInfo.numFaces = 6u;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture1* tex = nullptr;
    KTX_error_code rc = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &tex);
    if (rc != KTX_SUCCESS || !tex)
    {
        LOG_ERROR("Failed to create KTX prefiltered cubemap (error %d)", (int)rc);
        return false;
    }

    // 每级 mip：尺寸减半，roughness = level/(mips-1)
    for (uint32_t level = 0; level < numMips; ++level)
    {
        uint32_t faceSize = std::max(1u, baseSize >> level);
        float roughness = (numMips > 1) ? (float)level / (float)(numMips - 1) : 0.0f;

        LOG_INFO("  Prefilter mip %u: %ux%u, roughness=%.3f ...", level, faceSize, faceSize, roughness);

        // 生成 6 面预过滤（GGX 重要性采样）
        std::vector<imagecodec::VImagePtr> prefilteredFaces;
        for (uint32_t f = 0; f < 6; ++f)
        {
            prefilteredFaces.push_back(PrefilterEnvMapFace(faces, roughness, f, faceSize, samples));
        }

        // RGB32Float -> RGBA32F
        std::vector<uint8_t> levelData;
        ConvertRGBFacesToRGBA32F(prefilteredFaces, faceSize, levelData);

        // 写入 KTX（每面独立 offset）
        const uint32_t kBytesPerPixel = 4 * sizeof(float);
        uint32_t rowBytes = faceSize * kBytesPerPixel;
        for (uint32_t f = 0; f < 6; ++f)
        {
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset(ktxTexture(tex), level, 0, f, &offset);
            const uint8_t* faceData = levelData.data() + (size_t)f * faceSize * faceSize * kBytesPerPixel;
            memcpy(ktxTexture_GetData(ktxTexture(tex)) + offset, faceData,
                   (size_t)faceSize * faceSize * kBytesPerPixel);
        }
        (void)rowBytes;
    }

    // 序列化
    ktx_uint8_t* ktxBytes = nullptr;
    ktx_size_t ktxSize = 0;
    ktxTexture_WriteToMemory(ktxTexture(tex), &ktxBytes, &ktxSize);

    outData.assign(ktxBytes, ktxBytes + ktxSize);

    free(ktxBytes);
    ktxTexture_Destroy(ktxTexture(tex));

    LOG_INFO("Prefiltered env map mip-chain KTX generated: %zu bytes", outData.size());
    return !outData.empty();
}

bool GenerateRGBA32FCubemap_Data(
    const std::vector<imagecodec::VImagePtr>& faces,
    std::vector<uint8_t>& outData)
{
    if (faces.size() != 6)
    {
        LOG_ERROR("GenerateRGBA32FCubemap requires exactly 6 faces");
        return false;
    }

    const uint32_t kGL_RGBA32F = 0x8814;
    const uint32_t kVK_R32G32B32A32_SFLOAT = 109;

    uint32_t width  = faces[0]->GetWidth();
    uint32_t height = faces[0]->GetHeight();

    // 强制单 mip（后续可由运行时/工具再降采样）。RGBA32F cubemap 单 mip 保证
    // 所有 level 都有数据，避免未填充 mip 被采样为 0。
    const uint32_t numMips = 1;

    ktxTextureCreateInfo createInfo = {};
    createInfo.glInternalformat = kGL_RGBA32F;
    createInfo.vkFormat = kVK_R32G32B32A32_SFLOAT;
    createInfo.baseWidth = width;
    createInfo.baseHeight = height;
    createInfo.baseDepth = 1u;
    createInfo.numDimensions = 2u;
    createInfo.numLevels = numMips;
    createInfo.numLayers = 1u;
    createInfo.numFaces = 6u;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture1* tex = nullptr;
    KTX_error_code rc = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &tex);
    if (rc != KTX_SUCCESS || !tex)
    {
        LOG_ERROR("Failed to create KTX RGBA32F cubemap (error %d)", (int)rc);
        return false;
    }

    // 转换 RGB32Float -> RGBA32F，写 base level 6 面
    std::vector<uint8_t> levelData;
    ConvertRGBFacesToRGBA32F(faces, width, levelData);

    const uint32_t kBytesPerPixel = 4 * sizeof(float);
    for (uint32_t f = 0; f < 6; ++f)
    {
        ktx_size_t offset = 0;
        ktxTexture_GetImageOffset(ktxTexture(tex), 0, 0, f, &offset);
        const uint8_t* faceData = levelData.data() + (size_t)f * width * height * kBytesPerPixel;
        memcpy(ktxTexture_GetData(ktxTexture(tex)) + offset, faceData,
               (size_t)width * height * kBytesPerPixel);
    }

    ktx_uint8_t* ktxBytes = nullptr;
    ktx_size_t ktxSize = 0;
    ktxTexture_WriteToMemory(ktxTexture(tex), &ktxBytes, &ktxSize);
    outData.assign(ktxBytes, ktxBytes + ktxSize);
    free(ktxBytes);
    ktxTexture_Destroy(ktxTexture(tex));

    LOG_INFO("RGBA32F cubemap KTX generated: %ux%u (1 mip, %zu bytes)", width, height, outData.size());
    return !outData.empty();
}

// ==================== 未压缩 2D KTX 输出（避免 BC7/BC6H 依赖） ====================

#include "Runtime/AssetProcess/source/TextureProcess/stb_image_resize2.h"

bool GenerateUncompressed2DKTX_Data(
    imagecodec::VImagePtr image, bool srgb,
    std::vector<uint8_t>& outData)
{
    if (!image || !image->GetImageData())
    {
        LOG_ERROR("GenerateUncompressed2DKTX: null image");
        return false;
    }

    const uint32_t kGL_RGBA8 = 0x8058;
    const uint32_t kGL_SRGB8_ALPHA8 = 0x8C43;

    // 统一为 4 通道 uint8 源数据
    uint32_t srcW = image->GetWidth();
    uint32_t srcH = image->GetHeight();

    // 源数据必须是 RGBA8/SRGB8_ALPHA8；若其它格式（RGB8 等）先转换
    std::vector<uint8_t> rgbaData;
    const uint8_t* srcPixels = image->GetImageData();
    imagecodec::ImagePixelFormat srcFmt = image->GetFormat();
    if (srcFmt == imagecodec::FORMAT_RGBA8 || srcFmt == imagecodec::FORMAT_SRGB8_ALPHA8)
    {
        uint32_t bytesPerPix = image->GetBytesPerPixels();
        if (bytesPerPix == 4)
        {
            rgbaData.assign(srcPixels, srcPixels + (size_t)srcW * srcH * 4);
        }
        else
        {
            // 异常通道数，退回转换
            srcFmt = imagecodec::FORMAT_RGB8;
        }
    }
    if (rgbaData.empty())
    {
        // RGB8 -> RGBA8
        uint32_t bytesPerPix = image->GetBytesPerPixels();
        rgbaData.resize((size_t)srcW * srcH * 4);
        for (uint32_t y = 0; y < srcH; ++y)
        {
            for (uint32_t x = 0; x < srcW; ++x)
            {
                const uint8_t* p = srcPixels + ((size_t)y * srcW + x) * bytesPerPix;
                uint8_t* d = rgbaData.data() + ((size_t)y * srcW + x) * 4;
                d[0] = p[0]; d[1] = p[1]; d[2] = p[2];
                d[3] = (bytesPerPix >= 4) ? p[3] : 255;
            }
        }
    }

    uint32_t numMips = imagecodec::ImageUtil::CalcNumMipLevels(srcW, srcH);

    ktxTextureCreateInfo createInfo = {};
    createInfo.glInternalformat = srgb ? kGL_SRGB8_ALPHA8 : kGL_RGBA8;
    createInfo.vkFormat = 0;    // v1 用 glInternalformat
    createInfo.baseWidth = srcW;
    createInfo.baseHeight = srcH;
    createInfo.baseDepth = 1u;
    createInfo.numDimensions = 2u;
    createInfo.numLevels = numMips;
    createInfo.numLayers = 1u;
    createInfo.numFaces = 1u;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture1* tex = nullptr;
    KTX_error_code rc = ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &tex);
    if (rc != KTX_SUCCESS || !tex)
    {
        LOG_ERROR("Failed to create KTX uncompressed texture (error %d)", (int)rc);
        return false;
    }

    // 写 base + 逐级降采样 mip（stbir 线性降采样）
    {
        ktx_size_t offset = 0;
        ktxTexture_GetImageOffset(ktxTexture(tex), 0, 0, 0, &offset);
        memcpy(ktxTexture_GetData(ktxTexture(tex)) + offset, rgbaData.data(), (size_t)srcW * srcH * 4);
    }

    uint32_t w = srcW, h = srcH;
    for (uint32_t m = 1; m < numMips; ++m)
    {
        uint32_t nw = std::max(1u, w >> 1);
        uint32_t nh = std::max(1u, h >> 1);
        std::vector<uint8_t> tmp((size_t)nw * nh * 4);

        stbir_resize(rgbaData.data(), (int)srcW, (int)srcH, 0,
                     tmp.data(), (int)nw, (int)nh, 0,
                     STBIR_RGBA, STBIR_TYPE_UINT8,
                     STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL);

        ktx_size_t offset = 0;
        ktxTexture_GetImageOffset(ktxTexture(tex), m, 0, 0, &offset);
        memcpy(ktxTexture_GetData(ktxTexture(tex)) + offset, tmp.data(), (size_t)nw * nh * 4);

        w = nw; h = nh;
    }

    ktx_uint8_t* ktxBytes = nullptr;
    ktx_size_t ktxSize = 0;
    ktxTexture_WriteToMemory(ktxTexture(tex), &ktxBytes, &ktxSize);
    outData.assign(ktxBytes, ktxBytes + ktxSize);
    free(ktxBytes);
    ktxTexture_Destroy(ktxTexture(tex));

    LOG_INFO("Uncompressed 2D KTX generated: %ux%u, %u mips, %s, %zu bytes",
             srcW, srcH, numMips, srgb ? "sRGB" : "linear", outData.size());
    return !outData.empty();
}

NS_ASSETPROCESS_END
