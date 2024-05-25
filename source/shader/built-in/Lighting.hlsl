//
//  Lighting.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/6.
//

#ifndef GNX_ENGINE_LIGHTING_INCLUDE_H
#define GNX_ENGINE_LIGHTING_INCLUDE_H

// 光照结构体，兼容点光源、聚光灯、平行光源
struct Light
{
    float3 Strength;    // 光源强度
    bool isSpot;        // 是否聚光灯
    float3 color;       // 光源颜色
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct LightIndirect
{
    half3 diffuse;
    half3 specular;
};

// 计算光照衰减的系数
float CalcAttenuation(float distance, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - distance) / (falloffEnd - falloffStart));
}

//---------------------------------------------------------------------------------------
// 计算方向光的衰减
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, float3 normal)
{
    // 方向光的方向是从光源指向物体，所以和法向量一样需要从物体指向光源，需要反号
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl * L.color;

    return lightStrength;
}

//---------------------------------------------------------------------------------------
// 计算点光源的强度衰减
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, float3 pos, float3 normal)
{
    // 自表面指向光源的向量
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.  这里到时候可以放在外面判断
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * L.color * ndotl;
    return lightStrength;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att * L.color;

    return lightStrength;
}

//---------------------------------------------------------------------------------------
// 计算聚光灯的衰减
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, float3 pos, float3 normal)
{
    // 自表面到光源的向量
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test. 这里到时候可以放在外面判断
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor * L.color;

    return lightStrength;
}

Light CreateLightInfo()
{
    // cbuffer cbLighting : register(b2)
    // {
    //     float4 _WorldSpaceLightPos;     //方向光: (world space direction, _WorldSpaceLightPos0.w = 0). Other lights: (world space position, _WorldSpaceLightPos0.w = 1).
    //     float4 _LightColor;             // 光的颜色
    //     float3 _Strength;                // 光的强度
    //     float _FalloffStart; // point/spot light only
    //     float _FalloffEnd;   // point/spot light only
    //     float _SpotPower;    // spot light only
    // }
    Light lightInfo;
    lightInfo.Strength = _Strength;
    lightInfo.color = _LightColor;
    lightInfo.FalloffStart = _FalloffStart;
    lightInfo.FalloffEnd = _FalloffEnd;
    lightInfo.Position = _WorldSpaceLightPos.xyz;

    return lightInfo;
}

#endif



