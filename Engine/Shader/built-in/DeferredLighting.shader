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

// ============================================================
// 阴影（PCSS）
// ============================================================
// ShadowMap 深度纹理 + 光源矩阵/参数
Texture2D gShadowMap;
SamplerState gShadowMapSam;

cbuffer cbShadow
{
    float4x4 _LightV;          // 光源视图矩阵
    float4x4 _LightP;          // 光源投影矩阵（Reverse-Z 正交）
    float4 _ShadowMapSize;     // (width, height, 1/width, 1/height)
    float4 _ShadowParams;      // (depthBias, normalBias, lightSize, filterRadius)
    float4 _ShadowFlags;       // (enable=1, 0, 0, 0)
};

// 低差异随机数（Poisson 旋转用）
float ShadowRand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// 把世界坐标变换到光源 NDC，返回 (shadowUV.xy, receiverForwardDepth)
// 说明：阴影深度使用 Reverse-Z（近=1 远=0），这里统一转为"正向"线性深度
// (近=0 远=1) 以便 PCSS 按常规方向推导（blocker 深度更小 = 更靠近光源）。
float3 WorldToShadowUV(float3 worldPos)
{
    float4 lightClip = mul(float4(worldPos, 1.0), _LightV);
    lightClip = mul(lightClip, _LightP);

    // 正交投影：clip.w == 1，NDC z 即反向深度（近=1 远=0）
    float2 shadowUV = lightClip.xy * 0.5 + 0.5;
    float reverseZ = lightClip.z;

    // 越界时返回无效标记（用 z = -1 表示）
    if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
    {
        return float3(shadowUV, -1.0);
    }

    // 转为正向深度：近=0 远=1
    float forwardDepth = 1.0 - reverseZ;
    return float3(shadowUV, forwardDepth);
}

// 从 ShadowMap 采样某 UV 处的正向深度（近=0 远=1）
float SampleShadowForwardDepth(float2 uv)
{
    float reverseZ = gShadowMap.Sample(gShadowMapSam, uv).r;
    return 1.0 - reverseZ;
}

// Blocker 搜索：在给定半径内，找到比接收点更靠近光源（正向深度更小）的遮挡者平均深度
// 返回平均 blocker 正向深度；找不到则返回 -1
float FindBlockerDepth(float2 uv, float receiverDepth, float searchRadius, float bias)
{
    const int kBlockerSamples = 16;
    const float PI2 = 6.28318530718;

    float blockerSum = 0.0;
    int blockerCount = 0;

    float angleOffset = ShadowRand(uv * 7.13 + 1.7);
    float radiusOffset = ShadowRand(uv * 3.71 + 0.5);

    for (int i = 0; i < kBlockerSamples; i++)
    {
        float angle = angleOffset + (float)i * (PI2 / (float)kBlockerSamples);
        // 泊松盘（均匀分布 + 随机半径），避免规则锯齿
        float r = (radiusOffset + (float)i / (float)kBlockerSamples) * searchRadius;
        float2 offset = float2(cos(angle), sin(angle)) * r;

        float2 sampleUV = uv + offset * _ShadowMapSize.zw;  // zw = 1/width, 1/height
        float sampleDepth = SampleShadowForwardDepth(sampleUV);

        // blocker = 比接收点更靠近光源（正向深度更小）
        if (sampleDepth < receiverDepth - bias)
        {
            blockerSum += sampleDepth;
            blockerCount++;
        }
    }

    return blockerCount > 0 ? blockerSum / (float)blockerCount : -1.0;
}

// PCSS 阴影可见度
// 返回 1.0 = 完全照亮，0.0 = 完全阴影
float ComputePCSSShadow(float3 worldPos, float3 normal, float3 lightDir)
{
    float3 shadowData = WorldToShadowUV(worldPos);
    float2 uv = shadowData.xy;
    float receiverDepth = shadowData.z;

    // 不在阴影贴图覆盖范围内 → 无阴影
    if (receiverDepth < 0.0)
    {
        return 1.0;
    }

    float bias = _ShadowParams.x;
    float normalBias = _ShadowParams.y;
    float lightSize = _ShadowParams.z;    // 光源大小（决定软阴影）
    float baseRadius = _ShadowParams.w;   // 基础搜索半径（纹素）

    // 法线偏移：沿法线把接收点向"靠近光源"的一侧推，缓解自阴影痤疮。
    // lightDir 是"表面→光源"方向，因此沿 normal 推离表面即靠近光源。
    // normalBias 以世界单位表示（C++ 端按 ShadowMap 正交覆盖范围/分辨率换算，
    // 覆盖 12 世界单位 / 2048 纹素 ≈ 0.006 单位每纹素；0.02 ≈ 3 纹素）。
    // 掠射角（nDotL 小）时痤疮更明显，用 lerp 适当放大偏移。
    float nDotL = saturate(dot(normal, lightDir));   // 表面→光源
    float biasScale = lerp(2.0f, 1.0f, nDotL);       // 掠射角放大偏移
    float3 worldPosBias = worldPos + normal * (normalBias * biasScale);

    float3 shadowDataBias = WorldToShadowUV(worldPosBias);
    uv = shadowDataBias.xy;
    receiverDepth = shadowDataBias.z;
    if (receiverDepth < 0.0)
    {
        return 1.0;
    }

    // PCSS 第一步：Blocker 搜索，得到平均 blocker 深度
    // 搜索半径随光源大小（正交阴影下近似恒定，用 lightSize 放大）
    float blockerSearchRadius = baseRadius * (0.5 + lightSize * 50.0);
    float avgBlockerDepth = FindBlockerDepth(uv, receiverDepth, blockerSearchRadius, bias);

    // 没有找到 blocker → 不在阴影中
    if (avgBlockerDepth < 0.0)
    {
        return 1.0;
    }

    // PCSS 第二步：估算半影大小（penumbra）
    // blocker 离接收点越近 → 阴影越锐利；越远 → 越软
    float penumbra = lightSize * (receiverDepth - avgBlockerDepth) / max(avgBlockerDepth, 1e-4);
    float pcfRadius = baseRadius * (0.3 + penumbra * 40.0);
    pcfRadius = clamp(pcfRadius, 0.5, baseRadius * 2.0);

    // PCSS 第三步：PCF 过滤
    const int kPCFSamples = 24;
    const float PI2 = 6.28318530718;

    float shadow = 0.0;
    float angleOffset = ShadowRand(uv * 11.31 + 2.9);
    float radiusOffset = ShadowRand(uv * 5.17 + 0.3);

    for (int i = 0; i < kPCFSamples; i++)
    {
        float angle = angleOffset + (float)i * (PI2 / (float)kPCFSamples);
        float r = (radiusOffset + (float)i / (float)kPCFSamples) * pcfRadius;
        float2 offset = float2(cos(angle), sin(angle)) * r;
        float2 sampleUV = uv + offset * _ShadowMapSize.zw;

        // 采样点越界视为无阴影（保持软边缘自然）
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
        {
            shadow += 1.0;
            continue;
        }

        float sampleDepth = SampleShadowForwardDepth(sampleUV);
        // sampleDepth 更小 = 更靠近光源 = 该采样点在光中
        shadow += (sampleDepth < receiverDepth - bias) ? 0.0 : 1.0;
    }

    return shadow / (float)kPCFSamples;
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

    // BRDF LUT uses U=NdotV and V=roughness, matching GenerateBRDFLUT.
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
    float shadowFactor = 1.0;
    
    if (_WorldSpaceLightPos.w < 0.5)
    {
        // 方向光：直接使用光照方向
        lightDir = normalize(_WorldSpaceLightPos.xyz);
        // 方向光没有距离衰减
        attenuation = 1.0;

        // PCSS 阴影（仅主方向光，需 ShadowMap 有效）
        if (_ShadowFlags.x > 0.5)
        {
            shadowFactor = ComputePCSSShadow(gBufferData.position, gBufferData.normal, lightDir);
        }
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
    Lo *= shadowFactor;
    
    // IBL 环境光（漫反射辐照度 + 预过滤镜面反射）
    float3 ambient = ComputeIBL(gBufferData);
    
    // 最终颜色 = 直接光 + IBL环境光 + 自发光（来自RT0）
    float3 finalColor = Lo + ambient + emissive;
    return float4(finalColor, 1.0);
}
