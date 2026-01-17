//
//  GBufferPBR.hlsl
//  GNXEngine
//
//  PBR材质的G-Buffer生成Shader
//

#include "StandardBRDF.hlsl"
#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 position : POSITION;        // World space position
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord0 : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
};

// 顶点着色器
VertexOut VS(appdata_tan vin)
{
    VertexOut vout;
    
    // Transform to world space
    float4 posW = mul(float4(vin.position, 1.0), MATRIX_M);
    vout.position = posW.xyz;

    // Transform to clip space
    posW = mul(posW, MATRIX_V);
    posW = mul(posW, MATRIX_P);
    vout.PosH = posW;
    vout.texCoord0 = vin.texcoord;

    // 法线转换到世界空间
    float3 norm = normalize(mul(vin.normal, MATRIX_Normal));
    vout.worldNormal = norm;
    
    // 切线方向转换到世界空间
    float3 tang = normalize(mul(vin.tangent, MATRIX_Normal));
    vout.tangent = float4(tang, vin.tangent.w);
    
    return vout;
}

// 纹理和采样器 - 与PBR.hlsl保持一致
Texture2D gDiffuseMap : register(t0);
SamplerState gDiffuseMapSam : register(s0);

Texture2D gNormalMap : register(t1);
SamplerState gNormalMapSam : register(s1);

Texture2D gMetalRoughMap : register(t2);
SamplerState gMetalRoughMapSam : register(s2);

Texture2D gEmissiveMap : register(t3);
SamplerState gEmissiveMapSam : register(s3);

Texture2D gAmbientMap : register(t4);
SamplerState gAmbientMapSam : register(s4);

// 像素着色器 - 生成G-Buffer
GBufferOutput PS(VertexOut pin)
{
    // 1. 采样所有材质纹理
    float4 Kd = gDiffuseMap.Sample(gDiffuseMapSam, pin.texCoord0);
    float3 albedo = Kd.rgb;
    float opacity = Kd.a;
    
    // 2. 法线映射
    float3 normalSample = gNormalMap.Sample(gNormalMapSam, pin.texCoord0).xyz;
    float3 n = normalize(pin.worldNormal);
    
    // 使用NormalSampleToWorldSpace转换法线
    float3 normal = NormalSampleToWorldSpace(normalSample, n, pin.tangent);
    
    // 3. 采样Metallic-Roughness纹理
    float4 mrSample = gMetalRoughMap.Sample(gMetalRoughMapSam, pin.texCoord0);
    float metallic = mrSample.r;
    float roughness = mrSample.g;
    
    // 4. 采样AO纹理
    float4 Kao = gAmbientMap.Sample(gAmbientMapSam, pin.texCoord0);
    float ao = Kao.r;
    
    // 5. 采样Emissive纹理
    float4 Ke = gEmissiveMap.Sample(gEmissiveMapSam, pin.texCoord0);
    float3 emissive = Ke.rgb;
    
    // 6. 打包到G-Buffer
    return PackGBuffer(albedo, opacity, normal, roughness, metallic, ao, emissive, pin.position);
}

// 变体版本：不使用法线贴图（优化）
GBufferOutput PS_NoNormalMap(VertexOut pin)
{
    float4 Kd = gDiffuseMap.Sample(gDiffuseMapSam, pin.texCoord0);
    float3 albedo = Kd.rgb;
    float opacity = Kd.a;
    
    float3 normal = normalize(pin.worldNormal);
    
    float4 mrSample = gMetalRoughMap.Sample(gMetalRoughMapSam, pin.texCoord0);
    float metallic = mrSample.r;
    float roughness = mrSample.g;
    
    float4 Kao = gAmbientMap.Sample(gAmbientMapSam, pin.texCoord0);
    float ao = Kao.r;
    
    float4 Ke = gEmissiveMap.Sample(gEmissiveMapSam, pin.texCoord0);
    float3 emissive = Ke.rgb;
    
    return PackGBuffer(albedo, opacity, normal, roughness, metallic, ao, emissive, pin.position);
}

// 变体版本：仅漫反射纹理（最简化）
GBufferOutput PS_DiffuseOnly(VertexOut pin)
{
    float4 Kd = gDiffuseMap.Sample(gDiffuseMapSam, pin.texCoord0);
    float3 albedo = Kd.rgb;
    float opacity = Kd.a;
    
    float3 normal = normalize(pin.worldNormal);
    float roughness = 0.5;  // 默认粗糙度
    float metallic = 0.0;   // 非金属
    float ao = 1.0;         // 无AO
    float3 emissive = float3(0.0, 0.0, 0.0);
    
    return PackGBuffer(albedo, opacity, normal, roughness, metallic, ao, emissive, pin.position);
}
