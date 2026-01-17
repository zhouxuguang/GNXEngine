//
//  GBufferDiffuse.hlsl
//  GNXEngine
//
//  简单漫反射材质的G-Buffer生成Shader（非PBR）
//

#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord0 : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
};

VertexOut VS(appdata_tan vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.position, 1.0), MATRIX_M);
    vout.position = posW.xyz;

    posW = mul(posW, MATRIX_V);
    posW = mul(posW, MATRIX_P);
    vout.PosH = posW;
    vout.texCoord0 = vin.texcoord;

    float3 norm = normalize(mul(vin.normal, MATRIX_Normal));
    vout.worldNormal = norm;
    
    return vout;
}

Texture2D gDiffuseMap : register(t0);
SamplerState gDiffuseMapSam : register(s0);

// 简单漫反射材质的G-Buffer生成
GBufferOutput PS(VertexOut pin)
{
    float4 Kd = gDiffuseMap.Sample(gDiffuseMapSam, pin.texCoord0);
    float3 albedo = Kd.rgb;
    float opacity = Kd.a;
    
    float3 normal = normalize(pin.worldNormal);
    float roughness = 1.0;   // 漫反射材质粗糙度较高
    float metallic = 0.0;    // 非金属
    float ao = 1.0;          // 无AO
    float3 emissive = float3(0.0, 0.0, 0.0);
    
    return PackGBuffer(albedo, opacity, normal, roughness, metallic, ao, emissive, pin.position);
}

// 带有基础颜色的漫反射版本
cbuffer MaterialCB : register(b3)
{
    float4 gBaseColor;
}

GBufferOutput PS_WithColor(VertexOut pin)
{
    float4 Kd = gDiffuseMap.Sample(gDiffuseMapSam, pin.texCoord0);
    float3 albedo = Kd.rgb * gBaseColor.rgb;
    float opacity = Kd.a * gBaseColor.a;
    
    float3 normal = normalize(pin.worldNormal);
    float roughness = 1.0;
    float metallic = 0.0;
    float ao = 1.0;
    float3 emissive = float3(0.0, 0.0, 0.0);
    
    return PackGBuffer(albedo, opacity, normal, roughness, metallic, ao, emissive, pin.position);
}
