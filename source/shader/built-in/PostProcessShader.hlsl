//
//  PostProcessShader.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/6.
//

#include "GNXEngineCommon.hlsl"

struct v2f
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 fsTrianglePosition(int vtx) 
{
  float x = -1.0 + float((vtx & 1) << 2);
  float y = -1.0 + float((vtx & 2) << 1);
  return float4(x, y, 0.0, 1.0);
}

float2 fsTriangleUV(int vtx) 
{
  float u = (vtx == 1) ? 2.0 : 0.0;
  float v = (vtx == 2) ? 2.0 : 0.0;
  return float2(u, v);
}

v2f VS(uint vertexID : SV_VertexID)
{
    v2f vout;

    vout.position = fsTrianglePosition(vertexID);  
    vout.texcoord = fsTriangleUV(vertexID);  
#ifdef TEXCOORD_FLIP
    vout.texcoord.y = 1.0 - vout.texcoord.y;
#endif
    return vout;
}

Texture2D texImage : register(t0, space0);
SamplerState texImageSam  : register(s0, space1);

float3 aces_approx(float3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

// The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
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

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(in float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);
    return color;
}

half4 PS(v2f pin) : SV_Target
{
    half4 texColor = texImage.Sample(texImageSam, pin.texcoord);

    // if (color.x > 1.0 || color.y > 1.0 || color.z > 1.0)
    // {
    //     return float4(1.0, 0.0, 0.0, 1.0);
    // }

    float3 color = ACESFilm(texColor.rgb);
    //color = texColor;
    //float3 color = ACESFitted(texColor) * 2.8f;

    color = LinearToGammaSpace(color);
    return float4(color, 1.0);
}
