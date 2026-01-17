//
//  GBufferCommon.hlsl
//  GNXEngine
//
//  G-Buffer通用定义和工具函数
//

#ifndef GNX_ENGINE_GBUFFER_COMMON_H
#define GNX_ENGINE_GBUFFER_COMMON_H

#include "GNXEngineCommon.hlsl"

// G-Buffer输出结构体 - 4个Render Targets
// RT0: Albedo (RGB) + Opacity (A)
// RT1: World Normal (RGB) + Roughness (A) - 使用法线编码
// RT2: Metallic (R) + AO (G) + Emissive (B) + Reserved (A)
// RT3: World Position (RGB) - 可选，也可以从深度重建
struct GBufferOutput
{
    float4 Albedo_Opacity : SV_Target0;      // RGB: Albedo, A: Opacity
    float4 Normal_Roughness : SV_Target1;    // RGB: World Normal (encoded), A: Roughness
    float4 Metallic_AO_Emissive : SV_Target2; // R: Metallic, G: AO, B: Emissive intensity, A: Reserved
    float4 Position : SV_Target3;            // RGB: World Position (可选)
};

// 法线编码/解码函数
// 使用球面坐标编码法线到 [0,1] 范围
float3 EncodeNormal(float3 normal)
{
    float2 enc = normalize(normal.xy) * (sqrt(normal.z * 0.5 + 0.5));
    enc = enc * 0.5 + 0.5;
    return float3(enc, 0.0);
}

float3 DecodeNormal(float3 enc)
{
    float4 nn = float4(enc.xy, 0.0, 0.0);
    nn = nn * 2.0 - 1.0;
    float l = dot(nn.xy, nn.xy);
    float z = sqrt(1.0 - l);
    float3 normal = float3(nn.xy, z);
    return normalize(normal);
}

// 另一种更精确的法线编码方法（使用octahedral encoding）
float3 EncodeNormalOctahedron(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * sign(n.xy);
    return n * 0.5 + 0.5;
}

float3 DecodeNormalOctahedron(float3 enc)
{
    enc = enc * 2.0 - 1.0;
    float3 n = enc;
    float2 absxy = abs(enc.xy);
    n.z = 1.0 - absxy.x - absxy.y;
    n.xy = n.z >= 0.0 ? n.xy : (1.0 - absxy.yx) * sign(enc.xy);
    return normalize(n);
}

// G-Buffer填充辅助函数
GBufferOutput PackGBuffer(float3 albedo, float opacity, float3 normal, float roughness, 
                          float metallic, float ao, float3 emissive, float3 position)
{
    GBufferOutput output;
    
    // RT0: Albedo + Opacity
    output.Albedo_Opacity = float4(albedo, opacity);
    
    // RT1: Normal + Roughness
    // 使用octahedral编码法线
    float3 encodedNormal = EncodeNormalOctahedron(normalize(normal));
    output.Normal_Roughness = float4(encodedNormal.rg, 0.0, roughness);
    
    // RT2: Metallic + AO + Emissive
    float emissiveIntensity = length(emissive);
    output.Metallic_AO_Emissive = float4(metallic, ao, emissiveIntensity, 1.0);
    
    // RT3: World Position
    output.Position = float4(position, 1.0);
    
    return output;
}

// 从G-Buffer中解码材质信息
struct GBufferData
{
    float3 albedo;
    float opacity;
    float3 normal;
    float roughness;
    float metallic;
    float ao;
    float3 emissive;
    float3 position;
};

GBufferData UnpackGBuffer(float4 albedoOpacity, float4 normalRoughness, 
                          float4 metallicAOEmissive, float4 position)
{
    GBufferData data;
    
    data.albedo = albedoOpacity.rgb;
    data.opacity = albedoOpacity.a;
    
    data.normal = DecodeNormalOctahedron(normalRoughness.rgb);
    data.roughness = normalRoughness.a;
    
    data.metallic = metallicAOEmissive.r;
    data.ao = metallicAOEmissive.g;
    float emissiveIntensity = metallicAOEmissive.b;
    data.emissive = data.albedo * emissiveIntensity; // 简化：用albedo乘以强度
    
    data.position = position.rgb;
    
    return data;
}

#endif // GNX_ENGINE_GBUFFER_COMMON_H
