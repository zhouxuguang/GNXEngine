//
//  GNXEngineCommon.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/30.
//

#include "GNXEngineVariables.hlsl"

#ifndef GNX_ENGINE_COMMON_INCLUEDGDGDF_H
#define GNX_ENGINE_COMMON_INCLUEDGDGDF_H

#define UNITY_PI            3.14159265359f
#define UNITY_TWO_PI        6.28318530718f
#define UNITY_FOUR_PI       12.56637061436f
#define UNITY_INV_PI        0.31830988618f
#define UNITY_INV_TWO_PI    0.15915494309f
#define UNITY_INV_FOUR_PI   0.07957747155f
#define UNITY_HALF_PI       1.57079632679f
#define UNITY_INV_HALF_PI   0.636619772367f


//应用程序的顶点结构
struct appdata_base
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct appdata_tan
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct appdata_skin
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
    uint4 boneIndex : BONEINDICES;
    float4 weight : WEIGHTS;
};

struct appdata_full
{
    float3 position : POSITION;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float4 texcoord : TEXCOORD0;
    float4 texcoord1 : TEXCOORD1;
    float4 texcoord2 : TEXCOORD2;
    float4 texcoord3 : TEXCOORD3;
    half4 color : COLOR;
};

inline float GammaToLinearSpaceExact(float value)
{
    if (value <= 0.04045F)
        return value / 12.92F;
    else if (value < 1.0F)
        return pow((value + 0.055F)/1.055F, 2.4F);
    else
        return pow(value, 2.2F);
}

//gamma和linear之间的转换
inline half3 GammaToLinearSpace(half3 sRGB)
{
    // Approximate version from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return sRGB * (sRGB * (sRGB * 0.305306011h + 0.682171111h) + 0.012522878h);

    // Precise version, useful for debugging.
    //return half3(GammaToLinearSpaceExact(sRGB.r), GammaToLinearSpaceExact(sRGB.g), GammaToLinearSpaceExact(sRGB.b));
}

inline float LinearToGammaSpaceExact(float value)
{
    if (value <= 0.0F)
        return 0.0F;
    else if (value <= 0.0031308F)
        return 12.92F * value;
    else if (value < 1.0F)
        return 1.055F * pow(value, 0.4166667F) - 0.055F;
    else
        return pow(value, 0.45454545F);
}

inline half3 LinearToGammaSpace(half3 linRGB)
{
    linRGB = max(linRGB, half3(0.h, 0.h, 0.h));
    // An almost-perfect approximation from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return max(1.055h * pow(linRGB, 0.416666667h) - 0.055h, 0.h);

    // Exact version, useful for debugging.
    //return half3(LinearToGammaSpaceExact(linRGB.r), LinearToGammaSpaceExact(linRGB.g), LinearToGammaSpaceExact(linRGB.b));
}

//坐标和方向的转换

// 将世界坐标空间中的一点转换到裁剪空间
inline float4 WorldToClipPos(in float3 pos)
{
    return mul(MATRIX_VP, float4(pos, 1.0));
}

// 将观察坐标空间中的一点转换到裁剪空间
inline float4 ViewToClipPos(in float3 pos)
{
    return mul(MATRIX_P, float4(pos, 1.0));
}

// 将物体空间中的一点转换到观察空间
inline float3 ObjectToViewPos(in float3 pos)
{
    return mul(MATRIX_V, mul(MATRIX_M, float4(pos, 1.0))).xyz;
}

// 将世界坐标中的一点转换到观察空间
inline float3 WorldToViewPos(in float3 pos)
{
    return mul(MATRIX_V, float4(pos, 1.0)).xyz;
}

// 将模型坐标空间中的方向变换到世界空间
inline float3 ObjectToWorldDir(in float3 dir)
{
    return normalize(mul((float3x3)MATRIX_M, dir));
}

// 将世界坐标空间中的方向变换到模型空间
inline float3 WorldToObjectDir(in float3 dir)
{
    return normalize(mul((float3x3)MATRIX_M_INV, dir));
}

// 将模型空间中的法向量变换到世界坐标空间
inline float3 ObjectToWorldNormal(in float3 norm)
{
    // mul(IT_M, norm) => mul(norm, I_M) => {dot(norm, I_M.col0), dot(norm, I_M.col1), dot(norm, I_M.col2)}
    return normalize(mul(norm, (float3x3)MATRIX_M_INV));
}

// 计算世界坐标中的点到相机的方向
inline float3 WorldSpaceViewDir(in float3 worldPos)
{
    return normalize(_WorldSpaceCameraPos.xyz - worldPos);
}

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
half LinearRgbToLuminance(half3 linearRgb)
{
    return dot(linearRgb, half3(0.2126729f,  0.7151522f, 0.0721750f));
}

//根据法线和切线生成由切线空间转换到模型空间的转换矩阵
float3x3 GetTangentRotation(float3 normal, float3 tangent)
{
    // Build orthonormal basis.
    float3 N = normalize(normal);
    float3 T = normalize(tangent - dot(tangent, N)*N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);
    return TBN;
}

// 带偏手性的计算版本
float3x3 GetTangentRotation2(float3 normal, float4 tangent)
{
    // Build orthonormal basis.
    float3 N = normalize(normal);
    float3 T = normalize(tangent.xyz - dot(tangent.xyz, N)*N);
    float3 B = normalize(cross(N, T)) * tangent.w;

    float3x3 TBN = float3x3(T, B, N);
    return TBN;
}

//---------------------------------------------------------------------------------------
// 法线贴图采样的数据从切线空间转换到全局空间，这里全局空间包括模型坐标系、世界坐标系、相机坐标系等
//---------------------------------------------------------------------------------------
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float4 tangentW)
{
	// Build orthonormal basis.
	float3 N = unitNormalW;
	float3 T = normalize(tangentW.xyz - dot(tangentW.xyz, N)*N);
	float3 B = cross(N, T) * tangentW.w;

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
	float3 bumpedNormalW = mul(normalMapSample, TBN);

	return bumpedNormalW;
}

// 法线解码相关的函数

inline snorm float3 UnpackNormalDXT5nm(snorm float4 packednormal)
{
    snorm float3 normal;
    normal.xy = packednormal.wy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
    return normal;
}

// Unpack normal as DXT5nm (1, y, 1, x) or BC5 (x, y, 0, 1)
// Note neutral texture like "bump" is (0, 0, 1, 1) to work with both plain RGB normal and DXT5nm/BC5
snorm float3 UnpackNormalmapRGorAG(snorm float4 packednormal)
{
    // This do the trick
    packednormal.x *= packednormal.w;

    snorm float3 normal;
    normal.xy = packednormal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));
    return normal;
}

inline snorm float3 UnpackNormal(snorm float4 packednormal)
{
#if defined(ENGINE_DXT5nm)
    return UnpackNormalmapRGorAG(packednormal);
#else
    return packednormal.xyz * 2 - 1;
#endif
}

// 将高精度数据编码到低精度数据的函数，主要用于不支持渲染到浮点纹理的时候

// https://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/

// Encoding/decoding [0..1) floats into 8 bit/channel RGBA. Note that 1.0 will not be encoded properly.
inline float4 EncodeFloatRGBA(float v)
{
    float4 kEncodeMul = float4(1.0, 255.0, 65025.0, 16581375.0);
    float kEncodeBit = 1.0/255.0;
    float4 enc = kEncodeMul * v;
    enc = frac (enc);
    enc -= enc.yzww * kEncodeBit;
    return enc;
}
inline float DecodeFloatRGBA(float4 enc)
{
    float4 kDecodeDot = float4(1.0, 1/255.0, 1/65025.0, 1/16581375.0);
    return dot( enc, kDecodeDot );
}

// Encoding/decoding [0..1) floats into 8 bit/channel RG. Note that 1.0 will not be encoded properly.
inline float2 EncodeFloatRG(float v)
{
    float2 kEncodeMul = float2(1.0, 255.0);
    float kEncodeBit = 1.0/255.0;
    float2 enc = kEncodeMul * v;
    enc = frac (enc);
    enc.x -= enc.y * kEncodeBit;
    return enc;
}

inline float DecodeFloatRG(float2 enc)
{
    float2 kDecodeDot = float2(1.0, 1/255.0);
    return dot( enc, kDecodeDot );
}

// LinearEyeDepth: 将深度纹理采样值转换为线性视角空间深度
// 对于 Reverse-Z: rawDepth 从 1(near) 到 0(far)
// 对于传统 Z: rawDepth 从 0(near) 到 1(far)
inline float LinearEyeDepth(float depth)
{
    // 无限远 Reverse-Z: 线性深度 = near / depth
    // _ZBufferParams.z = 1/near, _ZBufferParams.w = 0
    // result = 1.0 / (depth/near + 0) = near / depth
#ifdef USE_REVERSE_Z
    return 1.0 / (_ZBufferParams.z * depth + _ZBufferParams.w);
#else
    // 传统 Z: result = 1.0 / ((1-far/near)/far * depth + (far/near)/far)
    return 1.0 / (_ZBufferParams.z * depth + _ZBufferParams.w);
#endif
}

// Linear01Depth: 将深度纹理采样值转换为 [0,1] 范围的线性深度
inline float Linear01Depth(float depth)
{
    // 将视角空间深度归一化到 [0,1]
    float eyeDepth = LinearEyeDepth(depth);
    float near = _ProjectionParams.y;
    float far = _ProjectionParams.z;
#ifdef USE_REVERSE_Z
    // 无限远 Reverse-Z: 用 near 作为归一化参考，超出 near~某个范围的值为 1
    return saturate(1.0 - near / eyeDepth);
#else
    return (eyeDepth - near) / (far - near);
#endif
}


// 从深度重建世界坐标
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    // if (depth <= 0.0001) return float3(0.0, 0.0, 0.0);
    // if (depth >= 0.9999) return float3(0.0, 0.0, 0.0);
    
#ifdef TEXCOORD_FLIP
    // 还原翻转，恢复真实的 NDC 方向
    uv.y = 1.0 - uv.y;
#endif
    
    // 计算NDC坐标
    float4 clipPos = float4(
        uv.x * 2.0f - 1.0f,   // [0,1] -> [-1,1]  XY需要转换
        uv.y * 2.0f - 1.0f,   // [0,1] -> [-1,1]  XY需要转换
        depth,                 // 已经是Vulkan NDC [0,1]，不需要转换！
        1.0f
    );

    // 计算相机空间坐标
    float4 camPos = mul(clipPos, MATRIX_INV_P);
    camPos.xyz /= camPos.w;
    //camPos.z *= -1;

    // 计算世界空间坐标
    float4 worldPos = mul(clipPos, MATRIX_INV_VP);
    worldPos.xyz /= worldPos.w;
    return worldPos.xyz;
}

// 从深度重建视图空间位置
float3 ReconstructViewPosition(float2 uv, float depth)
{
#ifdef TEXCOORD_FLIP
    // 还原翻转，恢复真实的 NDC 方向
    uv.y = 1.0 - uv.y;
#endif
    
    // 计算NDC坐标
    float4 clipPos = float4(
        uv.x * 2.0f - 1.0f,     // [0,1] -> [-1,1]
        uv.y * 2.0f - 1.0f,     // [0,1] -> [-1,1]
        depth,                  
        1.0f
    );
    
    // 逆投影到视图空间
    float4 viewPos = mul(clipPos, MATRIX_INV_P);
    viewPos.xyz /= viewPos.w;
    
    return viewPos.xyz;
}

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
#ifdef TEXCOORD_FLIP
    v = 1.0 - v;
#endif
    return float2(u, v);
}

#endif
