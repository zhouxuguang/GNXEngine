//
//  StandardBRDF.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/2.
//

#include "GNXEngineCommon.hlsl"

#ifndef GNX_ENGINE_STANDARDBRDF_INCLUDE_H
#define GNX_ENGINE_STANDARDBRDF_INCLUDE_H

//-----------------------------------------------------------------------------
// Helper to convert smoothness to roughness
//-----------------------------------------------------------------------------

half PerceptualRoughnessToRoughness(half perceptualRoughness)
{
    return perceptualRoughness * perceptualRoughness;
}

half RoughnessToPerceptualRoughness(half roughness)
{
    return sqrt(roughness);
}

// Smoothness is the user facing name
// it should be perceptualSmoothness but we don't want the user to have to deal with this name
half SmoothnessToRoughness(half smoothness)
{
    return (1 - smoothness) * (1 - smoothness);
}

half SmoothnessToPerceptualRoughness(half smoothness)
{
    return (1 - smoothness);
}

//-------------------------------------------------------------------------------------

inline half Pow4 (half x)
{
    return x*x*x*x;
}

inline half2 Pow4 (half2 x)
{
    return x*x*x*x;
}

inline half3 Pow4 (half3 x)
{
    return x*x*x*x;
}

inline half4 Pow4 (half4 x)
{
    return x*x*x*x;
}

// Pow5 uses the same amount of instructions as generic pow(), but has 2 advantages:
// 1) better instruction pipelining
// 2) no need to worry about NaNs
inline half Pow5 (half x)
{
    return x*x * x*x * x;
}

inline half2 Pow5 (half2 x)
{
    return x*x * x*x * x;
}

inline half3 Pow5 (half3 x)
{
    return x*x * x*x * x;
}

inline half4 Pow5 (half4 x)
{
    return x*x * x*x * x;
}

inline half3 FresnelTerm (half3 F0, half cosA)
{
    half t = Pow5 (1 - cosA);   // ala Schlick interpoliation
    return F0 + (1-F0) * t;
}
inline half3 FresnelLerp (half3 F0, half3 F90, half cosA)
{
    half t = Pow5 (1 - cosA);   // ala Schlick interpoliation
    return lerp(F0, F90, t);
}
// approximage Schlick with ^4 instead of ^5
inline half3 FresnelLerpFast(half3 F0, half3 F90, half cosA)
{
    half t = Pow4 (1 - cosA);
    return lerp(F0, F90, t);
}

// Note: Disney diffuse 需要乘以 diffuseAlbedo / PI. This is done outside of this function.
half DisneyDiffuse(half NdotV, half NdotL, half LdotH, half perceptualRoughness)
{
    half fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
    // Two schlick fresnel term
    half lightScatter   = (1 + (fd90 - 1) * Pow5(1 - NdotL));
    half viewScatter    = (1 + (fd90 - 1) * Pow5(1 - NdotV));

    return lightScatter * viewScatter;
}

// NOTE: Visibility term here is the full form from Torrance-Sparrow model, it includes Geometric term: V = G / (N.L * N.V)
// This way it is easier to swap Geometric terms and more room for optimizations (except maybe in case of CookTorrance geom term)

// Generic Smith-Schlick visibility term
inline half SmithVisibilityTerm(half NdotL, half NdotV, half k)
{
    half gL = NdotL * (1-k) + k;
    half gV = NdotV * (1-k) + k;
    return f32tof16(1.0) / (gL * gV + 1e-5f); // This function is not intended to be running on Mobile,
                                    // therefore epsilon is smaller than can be represented by half
}

// Smith-Schlick derived for Beckmann
inline half SmithBeckmannVisibilityTerm(half NdotL, half NdotV, half roughness)
{
    half c = 0.797884560802865h; // c = sqrt(2 / Pi)
    half k = roughness * c;
    return SmithVisibilityTerm (NdotL, NdotV, k) * 0.25f; // * 0.25 is the 1/4 of the visibility term
}

// Ref: http://jcgt.org/published/0003/02/03/paper.pdf，实现的G项
inline half SmithJointGGXVisibilityTerm(half NdotL, half NdotV, half roughness)
{
#if 0
    // Original formulation:
    //  lambda_v    = (-1 + sqrt(a2 * (1 - NdotL2) / NdotL2 + 1)) * 0.5f;
    //  lambda_l    = (-1 + sqrt(a2 * (1 - NdotV2) / NdotV2 + 1)) * 0.5f;
    //  G           = 1 / (1 + lambda_v + lambda_l);

    // Reorder code to be more optimal
    half a          = roughness;
    half a2         = a * a;

    half lambdaV    = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    half lambdaL    = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);

    // Simplify visibility term: (2.0f * NdotL * NdotV) /  ((4.0f * NdotL * NdotV) * (lambda_v + lambda_l + 1e-5f));
    return 0.5f / (lambdaV + lambdaL + 1e-5f);  // This function is not intended to be running on Mobile,
                                                // therefore epsilon is smaller than can be represented by half
#else
    // Approximation of the above formulation (simplify the sqrt, not mathematically correct but close enough)
    half a = roughness;
    half lambdaV = NdotL * (NdotV * (1 - a) + a);
    half lambdaL = NdotV * (NdotL * (1 - a) + a);

    return 0.5f / (lambdaV + lambdaL + 1e-5f);
#endif
}

// 标准微表面模型的实现

inline half DistributionGGX(half NdotH, float roughness)
{
    half a = roughness;
    half a2 = a*a;
    half NdotH2 = NdotH*NdotH;

    half num   = a2;
    half denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = UNITY_PI * denom * denom;

    return num / denom;
}

inline float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

inline float GeometrySmithSchlick(float NdotV, float roughness)
{
    float r = roughness;
    float k = (r*r) / 2.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return 1.0 / denom;
}

//法线分布函数，实现的D项
inline half GGXTerm(half NdotH, half roughness)
{
    half a2 = roughness * roughness;
    half d = (NdotH * a2 - NdotH) * NdotH + 1.0f; // 2 mad
    return UNITY_INV_PI * a2 / (d * d + 1e-7f); // This function is not intended to be running on Mobile,
                                            // therefore epsilon is smaller than what can be represented by half
}

inline half PerceptualRoughnessToSpecPower (half perceptualRoughness)
{
    half m = PerceptualRoughnessToRoughness(perceptualRoughness);   // m is the true academic roughness.
    half sq = max(1e-4f, m*m);
    half n = (2.0 / sq) - 2.0;                          // https://dl.dropboxusercontent.com/u/55891920/papers/mm_brdf.pdf
    n = max(n, 1e-4f);                                  // prevent possible cases of pow(0,0), which could happen when roughness is 1.0 and NdotH is zero
    return n;
}

// BlinnPhong normalized as normal distribution function (NDF)
// for use in micro-facet model: spec=D*G*F
// eq. 19 in https://dl.dropboxusercontent.com/u/55891920/papers/mm_brdf.pdf
inline half NDFBlinnPhongNormalizedTerm(half NdotH, half n)
{
    // norm = (n+2)/(2*pi)
    half normTerm = (n + 2.0) * (0.5/UNITY_PI);

    half specTerm = pow (NdotH, n);
    return specTerm * normTerm;
}

inline half3 Unity_SafeNormalize(half3 inVec)
{
    half dp3 = max(0.001f, dot(inVec, inVec));
    return inVec * rsqrt(dp3);
}

// 标准的IBL实现

// Based on: https://github.com/KhronosGroup/glTF-WebGL-PBR/blob/master/shaders/pbr-frag.glsl

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float3 reflectance0;            // full reflectance color (normal incidence angle)
	float3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	float3 diffuseColor;            // color contribution from diffuse lighting
	float3 specularColor;           // color contribution from specular lighting
	float3 n;								// normal at surface point
	float3 v;								// vector from surface point to camera
};

float4 SRGBtoLINEAR(float4 srgbIn)
{
	float3 linOut = pow(srgbIn.xyz, float3(2.2, 2.2, 2.2));

	return float4(linOut, srgbIn.a);
}

TextureCube texEnvMap : register(t5);
SamplerState texEnvSamp  : register(s5);

TextureCube texEnvMapIrradiance : register(t6);
SamplerState texEnvMapIrradianceSamp  : register(s6);

Texture2D texBRDF_LUT : register(t7);
SamplerState texBRDF_LUTSamp  : register(s7);

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
float3 getIBLContribution(PBRInfo pbrInputs, float3 n, float3 reflection)
{
    // 获得图像的mipmap层级数
    uint width, height, levels;
    texEnvMap.GetDimensions(0, width, height, levels);

	float mipCount = float(levels);
	float lod = pbrInputs.perceptualRoughness * mipCount;
	// retrieve a scale and bias to F0. See [1], Figure 3
	float2 brdfSamplePoint = clamp(float2(pbrInputs.NdotV, pbrInputs.perceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
	float3 brdf = texBRDF_LUT.SampleLevel(texBRDF_LUTSamp, brdfSamplePoint, 0).rgb;

	// HDR envmaps are already linear
	float3 diffuseLight = texEnvMapIrradiance.Sample(texEnvMapIrradianceSamp, n.xyz).rgb;
	float3 specularLight = texEnvMap.SampleLevel(texEnvSamp, reflection.xyz, lod).rgb;

	float3 diffuse = diffuseLight * pbrInputs.diffuseColor;
	float3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

	return diffuse + specular;
}

// Disney Implementation of diffuse from Physically-Based Shading at Disney by Brent Burley. See Section 5.3.
// http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
float3 diffuseBurley(PBRInfo pbrInputs)
{
	float f90 = 2.0 * pbrInputs.LdotH * pbrInputs.LdotH * pbrInputs.alphaRoughness - 0.5;

	return (pbrInputs.diffuseColor / UNITY_PI) * (1.0 + f90 * pow((1.0 - pbrInputs.NdotL), 5.0)) * (1.0 + f90 * pow((1.0 - pbrInputs.NdotV), 5.0));
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
float3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float rSqr = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(rSqr + (1.0 - rSqr) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(rSqr + (1.0 - rSqr) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (UNITY_PI * f * f);
}

float3 calculatePBRInputsMetallicRoughness(float4 albedo, float3 normal, float3 cameraPos, float3 worldPos, float4 mrSample, out PBRInfo pbrInputs )
{
	float perceptualRoughness = 1.0;
	float metallic = 1.0;

	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	perceptualRoughness = mrSample.g * perceptualRoughness;
	metallic = mrSample.r * metallic;

	const float c_MinRoughness = 0.04;

	perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
	metallic = clamp(metallic, 0.0, 1.0);
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness [2].
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// The albedo may be defined from a base texture or a flat color
	float4 baseColor = albedo;

	float3 f0 = float3(0.04, 0.04, 0.04);
	float3 diffuseColor = baseColor.rgb * (float3(1.0, 1.0, 1.0) - f0);
	diffuseColor *= 1.0 - metallic;
	float3 specularColor = lerp(f0, baseColor.rgb, metallic);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	float3 specularEnvironmentR0 = specularColor.rgb;
	float3 specularEnvironmentR90 = float3(1.0, 1.0, 1.0) * reflectance90;

	float3 n = normalize(normal);					// normal at surface point
	float3 v = normalize(cameraPos - worldPos);	// Vector from surface point to camera
	float3 reflection = -normalize(reflect(v, n));

	pbrInputs.NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	pbrInputs.perceptualRoughness = perceptualRoughness;
	pbrInputs.reflectance0 = specularEnvironmentR0;
	pbrInputs.reflectance90 = specularEnvironmentR90;
	pbrInputs.alphaRoughness = alphaRoughness;
	pbrInputs.diffuseColor = diffuseColor;
	pbrInputs.specularColor = specularColor;
	pbrInputs.n = n;
	pbrInputs.v = v;

	// Calculate lighting contribution from image based lighting source (IBL)
	float3 color = getIBLContribution(pbrInputs, n, reflection);

    //color = float3(0.01, 0.01, 0.01);

	return color;
}

float3 calculatePBRLightContribution(inout PBRInfo pbrInputs, float3 lightDirection, float3 lightColor )
{
	float3 n = pbrInputs.n;
	float3 v = pbrInputs.v;
	float3 l = normalize(lightDirection);	// Vector from surface point to light
	float3 h = normalize(l+v);				// Half vector between both l and v

	float NdotV = pbrInputs.NdotV;
	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	pbrInputs.NdotL = NdotL;
	pbrInputs.NdotH = NdotH;
	pbrInputs.LdotH = LdotH;
	pbrInputs.VdotH = VdotH;

	// Calculate the shading terms for the microfacet specular shading model
	float3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	// Calculation of analytical lighting contribution
	float3 diffuseContrib = (1.0 - F) * diffuseBurley(pbrInputs);
	float3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	float3 color = NdotL * lightColor * (diffuseContrib + specContrib);

	return color;
}

// http://www.thetenthplanet.de/archives/1180
// modified to fix handedness of the resulting cotangent frame
float3x3 cotangentFrame(float3 N, float3 p, float2 uv)
{
	// get edge vectors of the pixel triangle
	float3 dp1 = ddx( p );
	float3 dp2 = ddy( p );
	float2 duv1 = ddx( uv );
	float2 duv2 = ddy( uv );

	// solve the linear system
	float3 dp2perp = cross( dp2, N );
	float3 dp1perp = cross( N, dp1 );
	float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );

	// calculate handedness of the resulting cotangent frame
	float w = (dot(cross(N, T), B) < 0.0) ? -1.0 : 1.0;

	// adjust tangent if needed
	T = T * w;

	return float3x3( T * invmax, B * invmax, N );
}

float3 perturbNormal(float3 n, float3 v, float3 normalSample, float2 uv)
{
	float3 map = normalize(2.0 * normalSample - float3(1.0, 1.0, 1.0));
	float3x3 TBN = cotangentFrame(n, v, uv);
	return normalize(mul(map, TBN));
}


#endif
