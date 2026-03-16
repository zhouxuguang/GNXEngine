//
//  DeferredLighting.hlsl
//  GNXEngine
//
//  延迟渲染的光照计算Shader
//

#include "StandardBRDF.hlsl"
#include "Lighting.hlsl"
#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// 全屏四边形顶点着色器
VertexOut VS(uint vertexID : SV_VertexID)
{
    VertexOut vout;
    
    vout.PosH = fsTrianglePosition(vertexID);  
    vout.texCoord = fsTriangleUV(vertexID);  
#ifdef TEXCOORD_FLIP
    vout.texCoord.y = 1.0 - vout.texCoord.y;
#endif
    
    return vout;
}

// G-Buffer纹理
Texture2D gGBufferSceneColor;
SamplerState gGBufferSceneColorSam;

Texture2D gGBufferA;
SamplerState gGBufferASam;

Texture2D gGBufferB;
SamplerState gGBufferBSam;

Texture2D gGBufferC;
SamplerState gGBufferCSam;

Texture2D gGBufferD;  // Position
SamplerState gGBufferDSam;

// 或者从深度重建位置
Texture2D gDepth;
SamplerState gDepthSam;

// 从深度重建世界坐标
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 worldPos = mul(MATRIX_INV_VP, clipPos);
    return worldPos.xyz / worldPos.w;
}

// 计算单个光源的贡献
float3 ComputeLighting(GBufferData gBufferData, float3 lightDir, float3 lightColor, float lightIntensity)
{
    // 标准PBR光照计算
    
    float3 N = normalize(gBufferData.normal);
    float3 V = normalize(_WorldSpaceCameraPos - gBufferData.position);
    float3 L = normalize(lightDir);
    float3 H = normalize(V + L);
    
    float nDotL = saturate(dot(N, L));
    float nDotV = saturate(dot(N, V));
    float nDotH = saturate(dot(N, H));
    float lDotH = saturate(dot(L, H));
    
    // 基础反射率
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, gBufferData.albedo, gBufferData.metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(nDotH, gBufferData.roughness);
    float G = GeometrySchlickGGX(nDotL, gBufferData.roughness) * GeometrySchlickGGX(nDotV, gBufferData.roughness);
    float3 F = FresnelTerm(F0, lDotH);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - gBufferData.metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * nDotV * nDotL + 0.001;
    float3 specular = numerator / denominator;
    
    // 漫反射
    float3 diffuse = (gBufferData.albedo / UNITY_PI);
    
    // Disney漫反射（更精确）
    float diffuseCoff = DisneyDiffuse(nDotV, nDotL, lDotH, gBufferData.roughness);
    
    float3 Lo = (diffuseCoff * kD * gBufferData.albedo / UNITY_PI + specular) * lightColor * lightIntensity * nDotL;
    
    return Lo;
}

// 像素着色器 - 延迟光照
float4 PS(VertexOut pin) : SV_Target0
{
    // 1. 从G-Buffer中读取材质数据
    float4 normal = gGBufferA.Sample(gGBufferASam, pin.texCoord);
    float4 metallicSpecularRoughness = gGBufferB.Sample(gGBufferBSam, pin.texCoord);
    float4 baseColorAO = gGBufferC.Sample(gGBufferCSam, pin.texCoord);
    
    // 从深度重建顶点世界坐标
    float depth = gDepth.Sample(gDepthSam, pin.texCoord).r;
    float4 position;
    position.xyz = ReconstructWorldPosition(pin.texCoord, depth);
    
    // 解包G-Buffer数据
    GBufferData gBufferData = UnpackGBuffer(float4(baseColorAO.rgb, 0.0f), float4(normal.rgb, metallicSpecularRoughness.b), 
                            float4(metallicSpecularRoughness.r, baseColorAO.a, 0.0f, 0.0f), position);
    
    // 计算光照
    float3 lightDir = _WorldSpaceLightPos.xyz - gBufferData.position;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // 衰减
    float attenuation = 1.0;
    if (distance > _FalloffEnd)
    {
        attenuation = 0.0;
    }
    else if (distance > _FalloffStart)
    {
        attenuation = saturate((_FalloffEnd - distance) / (_FalloffEnd - _FalloffStart));
    }
    
    float3 Lo = ComputeLighting(gBufferData, lightDir, _LightColor.rgb, _Strength.x);
    Lo *= attenuation;
    
    // 环境光
    float3 ambient = float3(0.03, 0.03, 0.03) * gBufferData.albedo * gBufferData.ao;
    
    // 自发光
    float3 finalColor = Lo + ambient + gBufferData.emissive;
    
    return float4(finalColor, 1.0);
}
