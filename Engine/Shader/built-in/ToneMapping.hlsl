//
//  ToneMapping.hlsl
//  GNXEngine
//
//  色调映射（HDR -> SDR）公共函数库，供后处理、FXAA 等屏幕空间效果复用。
//

#ifndef GNX_ENGINE_TONE_MAPPING_H
#define GNX_ENGINE_TONE_MAPPING_H

#include "GNXEngineCommon.hlsl"

// 简化的 ACES filmic 近似（Krzysztof Narkowicz）
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// 带预曝光系数的 ACES 近似
float3 ACESFilmicApprox(float3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
}

// ACES fitted 曲线（Stephen Hill），精度更高：sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);
    color = RRTAndODTFit(color);
    color = mul(ACESOutputMat, color);
    return saturate(color);
}

// John Hable 的 filmic 曲线（Uncharted 2）
float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    return color;
}

// 统一的场景颜色色调映射入口：HDR 线性颜色 -> SDR 显示颜色（伽马编码后）。
// alpha < 0.5 视为 unlit 图像，其颜色已是最终线性 SDR，只做伽马编码。
float3 TonemapSceneColor(float4 hdrColor)
{
    if (hdrColor.a < 0.5h)
    {
        float3 sRGB = float3(LinearToGammaSpaceExact(hdrColor.r),
                             LinearToGammaSpaceExact(hdrColor.g),
                             LinearToGammaSpaceExact(hdrColor.b));
        return sRGB;
    }

    return LinearToGammaSpace(ACESFilm(hdrColor.rgb));
}

#endif // GNX_ENGINE_TONE_MAPPING_H
