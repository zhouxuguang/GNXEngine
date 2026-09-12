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

// 长度单位（米），与 Atmosphere::kLengthUnitInMeters 保持一致
#ifndef GNX_ATMOSPHERE_LENGTH_UNIT
#define GNX_ATMOSPHERE_LENGTH_UNIT 1000.0
#endif

// ---- 大气渲染参数（每帧更新） ----
cbuffer AtmosphereViewCB : register(b1)
{
    float4x4 inv_view_proj;        // 逆视图投影矩阵
    float4   camera_pos_exposure;  // xyz = 相机世界坐标(大气单位), w = 曝光
    float4   earth_center_pad;     // xyz = 地球中心(大气单位)
    float4   sun_direction_pad;    // xyz = 太阳方向(单位向量)
    float4   sun_size_pad;         // xy = (tan(sunAngularRadius), cos(sunAngularRadius))
    float4   white_point_pad;      // xyz = 白点
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

float3 GetSkyRadianceToPoint(float3 camera, float3 pos, float shadow_length,
                             float3 sun_direction, out float3 transmittance)
{
    return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture, transmittance_textureSam,
                                 scattering_texture, scattering_textureSam,
                                 single_mie_scattering_texture, single_mie_scattering_textureSam,
                                 camera, pos, shadow_length, sun_direction, transmittance);
}

float3 GetSunAndSkyIrradiance(float3 p, float3 normal, float3 sun_direction,
                              out float3 sky_irradiance)
{
    return GetSunAndSkyIrradiance(ATMOSPHERE, transmittance_texture, transmittance_textureSam,
                                  irradiance_texture, irradiance_textureSam,
                                  p, normal, sun_direction, sky_irradiance);
}

// ---- 额外几何体（球体/地面），与参考 Demo 一致 ----
static const float3 kSphereCenter = float3(0.0, 0.0, 1000.0) / GNX_ATMOSPHERE_LENGTH_UNIT;
static const float  kSphereRadius = 1000.0 / GNX_ATMOSPHERE_LENGTH_UNIT;
static const float3 kSphereAlbedo = float3(0.8, 0.8, 0.8);
static const float3 kGroundAlbedo = float3(0.0, 0.0, 0.04);

float GetSunVisibility(float3 pos, float3 sun_direction)
{
    float3 p = pos - kSphereCenter;
    float p_dot_v = dot(p, sun_direction);
    float p_dot_p = dot(p, p);
    float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
    float distance_to_intersection = -p_dot_v - sqrt(kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance);
    if (distance_to_intersection > 0.0)
    {
        float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_distance);
        float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
        return smoothstep(1.0, 0.0, ray_sphere_angular_distance / sun_size_pad.x);
    }
    return 1.0;
}

float GetSkyVisibility(float3 pos)
{
    float3 p = pos - kSphereCenter;
    float p_dot_p = dot(p, p);
    return 1.0 + p.z / sqrt(p_dot_p) * kSphereRadius * kSphereRadius / p_dot_p;
}

void GetSphereShadowInOut(float3 view_direction, float3 sun_direction,
                          float3 camera, out float d_in, out float d_out)
{
    float3 pos = camera - kSphereCenter;
    float pos_dot_sun = dot(pos, sun_direction);
    float view_dot_sun = dot(view_direction, sun_direction);
    float k = sun_size_pad.x;
    float l = 1.0 + k * k;
    float a = 1.0 - l * view_dot_sun * view_dot_sun;
    float b = dot(pos, view_direction) - l * pos_dot_sun * view_dot_sun - k * kSphereRadius * view_dot_sun;
    float c = dot(pos, pos) - l * pos_dot_sun * pos_dot_sun -
              2.0 * k * kSphereRadius * pos_dot_sun - kSphereRadius * kSphereRadius;
    float discriminant = b * b - a * c;
    if (discriminant > 0.0)
    {
        d_in  = max(0.0, (-b - sqrt(discriminant)) / a);
        d_out = (-b + sqrt(discriminant)) / a;
        float d_base = -pos_dot_sun / view_dot_sun;
        float d_apex = -(pos_dot_sun + kSphereRadius / k) / view_dot_sun;
        if (view_dot_sun > 0.0)
        {
            d_in  = max(d_in, d_apex);
            d_out = a > 0.0 ? min(d_out, d_base) : d_base;
        }
        else
        {
            d_in  = a > 0.0 ? max(d_in, d_base) : d_base;
            d_out = min(d_out, d_apex);
        }
    }
    else
    {
        d_in  = 0.0;
        d_out = 0.0;
    }
}

[shader("pixel")]
float4 PS(VS_OUTPUT input) : SV_Target0
{
    float3 camera = camera_pos_exposure.xyz;
    float  exposure = camera_pos_exposure.w;
    float3 earth_center = earth_center_pad.xyz;
    float3 sun_direction = normalize(sun_direction_pad.xyz);
    float2 sun_size = sun_size_pad.xy;
    float3 white_point = white_point_pad.xyz;

    // 视线重建：取深度范围内一个有限值 (z=0.5) 反投影得到世界坐标点。
    // 注意：Reverse-Z 下远平面对应 NDC z=0，在无限远投影里等价于“无穷远”，
    // 反投影后齐次坐标 w=0，会得到 Inf/NaN（这正是之前天空全黑的根因）。
    float2 uv = input.uv;
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    float4 clipPos = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0.5, 1.0);
    float4 worldFar = mul(clipPos, inv_view_proj);
    worldFar /= worldFar.w;

    float3 ray = worldFar.xyz - camera;
    float3 view_direction = normalize(ray);

    // 本片段张角（用于球体解析抗锯齿）
    float fragment_angular_size = length(ddx(ray) + ddy(ray)) / length(ray);

    float shadow_in;
    float shadow_out;
    GetSphereShadowInOut(view_direction, sun_direction, camera, shadow_in, shadow_out);

    // 太阳接近地平线时淡出光柱
    float lightshaft_fadein_hack = smoothstep(0.02, 0.04, dot(normalize(camera - earth_center), sun_direction));

    // ---- 球体 ----
    float3 p = camera - kSphereCenter;
    float p_dot_v = dot(p, view_direction);
    float p_dot_p = dot(p, p);
    float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
    float discriminant = kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance;
    float sphere_alpha = 0.0;
    float3 sphere_radiance = float3(0.0, 0.0, 0.0);
    if (discriminant >= 0.0)
    {
        float distance_to_intersection = -p_dot_v - sqrt(discriminant);
        if (distance_to_intersection > 0.0)
        {
            float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_distance);
            float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
            sphere_alpha = min(ray_sphere_angular_distance / fragment_angular_size, 1.0);

            float3 pos = camera + view_direction * distance_to_intersection;
            float3 normal = normalize(pos - kSphereCenter);
            float3 sky_irradiance;
            float3 sun_irradiance = GetSunAndSkyIrradiance(pos - earth_center, normal, sun_direction, sky_irradiance);
            sphere_radiance = kSphereAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);

            float shadow_length = max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) * lightshaft_fadein_hack;
            float3 transmittance;
            float3 in_scatter = GetSkyRadianceToPoint(camera - earth_center, pos - earth_center,
                                                      shadow_length, sun_direction, transmittance);
            sphere_radiance = sphere_radiance * transmittance + in_scatter;
        }
    }

    // ---- 地面 ----
    p = camera - earth_center;
    p_dot_v = dot(p, view_direction);
    p_dot_p = dot(p, p);
    float ray_earth_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
    discriminant = dot(earth_center, earth_center) - ray_earth_center_squared_distance;
    float ground_alpha = 0.0;
    float3 ground_radiance = float3(0.0, 0.0, 0.0);
    if (discriminant >= 0.0)
    {
        float distance_to_intersection = -p_dot_v - sqrt(discriminant);
        if (distance_to_intersection > 0.0)
        {
            float3 pos = camera + view_direction * distance_to_intersection;
            float3 normal = normalize(pos - earth_center);
            float3 sky_irradiance;
            float3 sun_irradiance = GetSunAndSkyIrradiance(pos - earth_center, normal, sun_direction, sky_irradiance);
            ground_radiance = kGroundAlbedo * (1.0 / PI) * (
                sun_irradiance * GetSunVisibility(pos, sun_direction) +
                sky_irradiance * GetSkyVisibility(pos));

            float shadow_length = max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) * lightshaft_fadein_hack;
            float3 transmittance;
            float3 in_scatter = GetSkyRadianceToPoint(camera - earth_center, pos - earth_center,
                                                      shadow_length, sun_direction, transmittance);
            ground_radiance = ground_radiance * transmittance + in_scatter;
            ground_alpha = 1.0;
        }
    }

    // ---- 天空 ----
    float shadow_length = max(0.0, shadow_out - shadow_in) * lightshaft_fadein_hack;
    float3 transmittance;
    float3 radiance = GetSkyRadiance(camera - earth_center, view_direction, shadow_length,
                                     sun_direction, transmittance);

    // 太阳圆盘
    if (dot(view_direction, sun_direction) > sun_size.y)
    {
        radiance = radiance + transmittance * GetSolarRadiance();
    }

    radiance = lerp(radiance, ground_radiance, ground_alpha);
    radiance = lerp(radiance, sphere_radiance, sphere_alpha);

    // 输出【线性 HDR 辐射度】，由引擎延迟渲染管线末端的 PostProcessing
    // （ACES 色调映射 + LinearToGammaSpace）统一完成色调映射。
    //
    // 参考 Demo 是把 1-exp(-L*exposure) 与 gamma 直接在天空着色器里做完并输出到屏幕；
    // 而本引擎管线末端已有后处理做同样的事，若这里再做一次就是【重复色调映射】，
    // 结果会明显发白、颜色偏灰（这正是与参考图差异大的根因）。
    return float4(radiance * exposure, 1.0);
}
