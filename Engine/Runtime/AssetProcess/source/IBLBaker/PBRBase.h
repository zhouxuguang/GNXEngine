//
//  PBRBase.h
//  GNXEngine
//
//  Created by zhouxuguang on 2026/2/12.
//

#ifndef GNX_ENGINE_PBR_BASE_INCLUDE
#define GNX_ENGINE_PBR_BASE_INCLUDE

#include "AssetProcessDefine.h"
#include "Runtime/ImageCodec/include/VImage.h"

NS_ASSETPROCESS_BEGIN

float RadicalInverse_VdC(uint32_t bits);

mathutil::Vector2f Hammersley(uint32_t i, uint32_t N);

mathutil::Vector3f ImportanceSampleGGX(const mathutil::Vector2f Xi,
                                       float roughness, const mathutil::Vector3f& N);

float GeometrySchlickGGX(float NdotV, float roughness);

mathutil::Vector2f IntegrateBRDF(float NdotV, float roughness, uint32_t samples);

imagecodec::VImagePtr GenerateBRDFLUT(uint32_t imageSize, uint32_t samples);


NS_ASSETPROCESS_END

#endif // GNX_ENGINE_PBR_BASE_INCLUDE
