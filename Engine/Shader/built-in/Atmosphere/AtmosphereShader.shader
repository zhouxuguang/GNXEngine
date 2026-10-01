//
//  AtmosphereShader.shader
//  GNXEngine
//
//  预计算大气散射 - 天空渲染 Pass
//  基于预计算得到的 LUT（透射率 / 单次+多次散射 / 辐照度）计算当前视线的天空辐射度，
//  并叠加太阳圆盘。以全屏三角形绘制，覆盖远平面（天空）区域。
//
//  参考: https://ebruneton.github.io/precomputed_atmospheric_scattering/
//

#include "AtmosphereCommon.hlsl"

// ---- 大气渲染参数（每帧由 AtmosphereComponent 传入） ----
cbuffer AtmosphereViewCB : register(b1)
{
    float4x4 inv_view_proj;          // 逆视图投影矩阵
    float4   camera_pos_exposure;    // xyz = 相机相对行星中心坐标(米), w = 曝光
    float4   sun_direction_pad;      // xyz = 太阳方向(单位向量)
    float4   sun_size_pad;           // xy = (tan(sunAngularRadius), cos(sunAngularRadius))
    float4   white_point_pad;        // xyz = 白点
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

// ---- 输入输出 ----
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 fsTrianglePosition(uint vtx)
{
    float x = -1.0 + float((vtx & 1) << 2);
    float y = -1.0 + float((vtx & 2) << 1);
    return float4(x, y, 0.0, 1.0);
}

// 与引擎 GNXEngineCommon.hlsl 中的 fsTriangleUV 保持一致（含 TEXCOORD_FLIP）
float2 fsTriangleUV(uint vtx)
{
    float u = (vtx == 1) ? 2.0 : 0.0;
    float v = (vtx == 2) ? 2.0 : 0.0;
#ifdef TEXCOORD_FLIP
    v = 1.0 - v;
#endif
    return float2(u, v);
}

[shader("vertex")]
VS_OUTPUT VS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    float4 pos = fsTrianglePosition(vertexID);

    // 将顶点放在远平面：Reverse-Z 时 NDC z=0，传统 Z 时 z=1
#ifdef USE_REVERSE_Z
    float farZ = 0.0;
#else
    float farZ = 1.0;
#endif

    output.position = float4(pos.x, pos.y, farZ, 1.0);
    output.uv = fsTriangleUV(vertexID);
    return output;
}

// ---- 纹理与采样器 ----
Texture2D  transmittance_texture;
SamplerState transmittance_textureSam;

Texture3D  scattering_texture;
SamplerState scattering_textureSam;

Texture3D  single_mie_scattering_texture;
SamplerState single_mie_scattering_textureSam;

Texture2D  irradiance_texture;
SamplerState irradiance_textureSam;

// ---- 便捷封装（把纹理/采样器补全后调用 AtmosphereCommon.hlsl 中的实现） ----
float3 GetSolarRadiance()
{
    return ATMOSPHERE.solar_irradiance /
           (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
}

float3 GetSkyRadiance(float3 camera, float3 view_ray, float shadow_length,
                      float3 sun_direction, out float3 transmittance)
{
    return GetSkyRadiance(ATMOSPHERE, transmittance_texture, transmittance_textureSam,
                          scattering_texture, scattering_textureSam,
                          single_mie_scattering_texture, single_mie_scattering_textureSam,
                          camera, view_ray, shadow_length, sun_direction, transmittance);
}

[shader("pixel")]
float4 PS(VS_OUTPUT input) : SV_Target0
{
    float2 uv = input.uv;
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    float4 clipPos = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0.5, 1.0);
    float4 rayPoint = mul(clipPos, inv_view_proj);
    float3 viewDirection = normalize(rayPoint.xyz / rayPoint.w);
    float3 sunDirection = normalize(sun_direction_pad.xyz);
    float3 transmittance;
    float3 radiance = GetSkyRadiance(camera_pos_exposure.xyz, viewDirection, 0.0,
                                      sunDirection, transmittance);
    if (dot(viewDirection, sunDirection) > sun_size_pad.y)
        radiance += transmittance * GetSolarRadiance();
    return float4(radiance * camera_pos_exposure.w, 1.0);
}
