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

// SSAO纹理
Texture2D gSSAO;
SamplerState gSSAOSam;

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
// IBL 环境光照：漫反射辐照度图 + 预过滤镜面反射图 + BRDF LUT
// 纹理 texEnvMapIrradiance / texEnvMap / texBRDF_LUT 在 StandardBRDF.hlsl 中声明，
// 由 DeferredLightingPass 按名称绑定（enableIBL 时）。
float3 ComputeIBL(GBufferData gBufferData)
{
    float3 N = normalize(gBufferData.normal);
    float3 V = normalize(_WorldSpaceCameraPos - gBufferData.position);
    float3 R = reflect(-V, N);

    float NdotV = saturate(dot(N, V));
    float perceptualRoughness = saturate(gBufferData.roughness);
    perceptualRoughness = max(perceptualRoughness, 0.045);   // 避免全光滑时 prefilter mip0 闪烁

    float metallic = saturate(gBufferData.metallic);

    // 反射率 F0：非金属用 0.04，金属用 albedo
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, gBufferData.albedo, metallic);

    float3 diffuseColor = gBufferData.albedo * (1.0 - metallic) * (1.0 - F0);
    float3 specularColor = lerp(F0, gBufferData.albedo, metallic);

    // 预过滤环境图 mip 数
    uint width, height, levels;
    texEnvMap.GetDimensions(0, width, height, levels);
    float mipCount = float(levels);

    float lod = perceptualRoughness * max(mipCount - 1.0, 0.0);

    // 采样漫反射辐照度 + 预过滤镜面
    float3 irradiance = texEnvMapIrradiance.Sample(texEnvMapIrradianceSam, N).rgb;
    float3 prefiltered = texEnvMap.SampleLevel(texEnvMapSam, R, lod).rgb;

    // BRDF LUT（split-sum 第二项）
    float2 brdfSample = clamp(float2(NdotV, perceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));
    float2 brdf = texBRDF_LUT.SampleLevel(texBRDF_LUTSam, brdfSample, 0).rg;

    float3 diffuseIBL = irradiance * diffuseColor;
    float3 specularIBL = prefiltered * (specularColor * brdf.x + brdf.y);

    return (diffuseIBL + specularIBL) * gBufferData.ao;
}

// 像素着色器 - 延迟光照
float4 PS(VertexOut pin) : SV_Target0
{
    // 1. 从G-Buffer中读取材质数据
    float4 normal = gGBufferA.Sample(gGBufferASam, pin.texCoord);
    float4 metallicSpecularRoughness = gGBufferB.Sample(gGBufferBSam, pin.texCoord);
    float4 baseColorAO = gGBufferC.Sample(gGBufferCSam, pin.texCoord);
    
    // 从RT0读取自发光颜色（BasePass写入）
    float3 emissive = gGBufferSceneColor.Sample(gGBufferSceneColorSam, pin.texCoord).rgb;
    
    // 从深度重建顶点世界坐标
    float depth = gDepth.Sample(gDepthSam, pin.texCoord).r;
    float4 position;
    position.xyz = ReconstructWorldPosition(pin.texCoord, depth);
    position.w = 1.0;
    
    // 解包G-Buffer数据（法线、albedo、metallic、roughness、ao）
    GBufferData gBufferData = UnpackGBuffer(float4(baseColorAO.rgb, 0.0f), float4(normal.rgb, metallicSpecularRoughness.b), 
                            float4(metallicSpecularRoughness.r, baseColorAO.a, 0.0f, 0.0f), position);
    
    // 判断光源类型并计算光照方向和衰减
    // w = 0: 方向光，xyz 是光照方向
    // w = 1: 点光源，xyz 是光源位置
    float3 lightDir;
    float attenuation = 1.0;
    
    if (_WorldSpaceLightPos.w < 0.5)
    {
        // 方向光：直接使用光照方向
        lightDir = normalize(_WorldSpaceLightPos.xyz);
        // 方向光没有距离衰减
        attenuation = 1.0;
    }
    else
    {
        // 点光源：从光源位置计算方向
        float3 lightVec = _WorldSpaceLightPos.xyz - gBufferData.position;
        float distance = length(lightVec);
        lightDir = lightVec / distance;
        
        // 距离衰减
        if (distance > _FalloffEnd)
        {
            attenuation = 0.0;
        }
        else if (distance > _FalloffStart)
        {
            attenuation = saturate((_FalloffEnd - distance) / (_FalloffEnd - _FalloffStart));
        }
    }
    
    float3 Lo = ComputeLighting(gBufferData, lightDir, _LightColor.rgb, _Strength.x);
    Lo *= attenuation;
    
    // IBL 环境光（漫反射辐照度 + 预过滤镜面反射）
    float3 ambient = ComputeIBL(gBufferData);
    
    // 最终颜色 = 直接光 + IBL环境光 + 自发光（来自RT0）
    float3 finalColor = Lo + ambient + emissive;
    return float4(finalColor, 1.0);
}
