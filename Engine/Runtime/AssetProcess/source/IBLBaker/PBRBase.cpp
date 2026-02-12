//
//  PBRBase.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2026/2/12.
//

#include "PBRBase.h"
#include "Runtime/MathUtil/include/HalfFloat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "TextureImporter.h"
#include <fstream>

NS_ASSETPROCESS_BEGIN

const float PI = 3.14159265358979323846264338327950288;

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
    
    uint16_t* pImageData = (uint16_t*)image->GetPixels();
    
    for (int x = 0; x < imageSize; x++)
    {
        for (int y = 0; y < imageSize; y++)
        {
            float NoV = (y + 0.5f) * (1.0f / imageSize);
            float roughness = (x + 0.5f) * (1.0f / imageSize);
            
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
        return false;
    }

    // 写入 ktx 数据
    outFile.write(reinterpret_cast<const char*>(ktxData.data()), ktxData.size());
    outFile.close();
}

NS_ASSETPROCESS_END
