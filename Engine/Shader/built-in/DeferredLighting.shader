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
    
    // 创建全屏三角形（3个顶点）
    float2 positions[3] = {
        float2(-1.0, 1.0),
        float2(3.0, 1.0),
        float2(-1.0, -3.0)
    };
    
    float2 texCoords[3] = {
        float2(0.0, 0.0),
        float2(2.0, 0.0),
        float2(0.0, 2.0)
    };
    
    vout.PosH = float4(positions[vertexID], 0.0, 1.0);
    vout.texCoord = texCoords[vertexID];
    
    return vout;
}

// G-Buffer纹理
Texture2D gGBuffer0 : register(t0);  // Albedo + Opacity
SamplerState gGBufferSam0 : register(s0);

Texture2D gGBuffer1 : register(t1);  // Normal + Roughness
SamplerState gGBufferSam1 : register(s1);

Texture2D gGBuffer2 : register(t2);  // Metallic + AO + Emissive
SamplerState gGBufferSam2 : register(s2);

Texture2D gGBuffer3 : register(t3);  // Position
SamplerState gGBufferSam3 : register(s3);

// 或者从深度重建位置
Texture2D gDepthTexture : register(t4);
SamplerState gDepthSam : register(s4);

// 注意：使用引擎统一的cbPerCamera和cbLighting（在GNXEngineVariables.hlsl中定义）
// cbPerCamera包含: MATRIX_INV_VP, _WorldSpaceCameraPos 等
// cbLighting包含: _WorldSpaceLightPos, _LightColor, _Strength, _FalloffStart, _FalloffEnd, _SpotPower

// 从深度重建世界坐标
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    #if defined(VULKAN)
    clipPos.y = -clipPos.y;  // Vulkan的Y轴翻转
    #endif
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
    float4 albedoOpacity = gGBuffer0.Sample(gGBufferSam0, pin.texCoord);
    float4 normalRoughness = gGBuffer1.Sample(gGBufferSam1, pin.texCoord);
    float4 metallicAOEmissive = gGBuffer2.Sample(gGBufferSam2, pin.texCoord);
    float4 position = gGBuffer3.Sample(gGBufferSam3, pin.texCoord);
    
    // 如果没有位置数据，从深度重建
    if (position.a < 0.5)
    {
        float depth = gDepthTexture.Sample(gDepthSam, pin.texCoord).r;
        position.rgb = ReconstructWorldPosition(pin.texCoord, depth);
    }
    
    // 解包G-Buffer数据
    GBufferData gBufferData = UnpackGBuffer(albedoOpacity, normalRoughness, metallicAOEmissive, position);
    
    // 2. 检查是否在场景范围内
    if (gBufferData.opacity < 0.01)
    {
        discard;  // 跳过透明像素
    }
    
    // 3. 计算光照
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
    
    // 光照计算
    float3 Lo = ComputeLighting(gBufferData, lightDir, _LightColor.rgb, _Strength.x);
    Lo *= attenuation;
    
    // 4. 环境光
    float3 ambient = float3(0.03, 0.03, 0.03) * gBufferData.albedo * gBufferData.ao;
    
    // 5. 自发光
    float3 finalColor = Lo + ambient + gBufferData.emissive;
    
    // 6. Tone mapping（可选）
    finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    
    // 7. Gamma校正
    finalColor = pow(finalColor, 1.0 / 2.2);
    
    return float4(finalColor, 1.0);
}

// 多光源版本（使用ComputeShader或 tiled lighting）
// 这里提供一个简单的多光源循环版本作为示例
static const int MAX_LIGHTS = 16;

cbuffer MultiLightCB : register(b6)
{
    int gLightCount;
    float3 __padding;
    
    // 简化的光源数组
    struct
    {
        float4 position;
        float4 color;
        float range;
    } gLights[MAX_LIGHTS];
}

float4 PS_MultiLights(VertexOut pin) : SV_Target0
{
    // 读取G-Buffer
    float4 albedoOpacity = gGBuffer0.Sample(gGBufferSam0, pin.texCoord);
    float4 normalRoughness = gGBuffer1.Sample(gGBufferSam1, pin.texCoord);
    float4 metallicAOEmissive = gGBuffer2.Sample(gGBufferSam2, pin.texCoord);
    float4 position = gGBuffer3.Sample(gGBufferSam3, pin.texCoord);
    
    if (position.a < 0.5)
    {
        float depth = gDepthTexture.Sample(gDepthSam, pin.texCoord).r;
        position.rgb = ReconstructWorldPosition(pin.texCoord, depth);
    }
    
    GBufferData gBufferData = UnpackGBuffer(albedoOpacity, normalRoughness, metallicAOEmissive, position);
    
    if (gBufferData.opacity < 0.01)
    {
        discard;
    }
    
    // 累积所有光源
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    for (int i = 0; i < gLightCount; i++)
    {
        float3 lightDir = gLights[i].position.xyz - gBufferData.position;
        float distance = length(lightDir);
        
        if (distance < gLights[i].range)
        {
            lightDir = normalize(lightDir);
            
            float attenuation = 1.0 - distance / gLights[i].range;
            attenuation = attenuation * attenuation;
            
            Lo += ComputeLighting(gBufferData, lightDir, gLights[i].color.rgb, 1.0) * attenuation;
        }
    }
    
    // 环境光
    float3 ambient = float3(0.03, 0.03, 0.03) * gBufferData.albedo * gBufferData.ao;
    
    // 最终颜色
    float3 finalColor = Lo + ambient + gBufferData.emissive;
    
    // Tone mapping
    finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    
    // Gamma校正
    finalColor = pow(finalColor, 1.0 / 2.2);
    
    return float4(finalColor, 1.0);
}
