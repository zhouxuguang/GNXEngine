#include "AtmosphereCommon.hlsl"

struct VS_INPUT
{
    float3 PosL    : POSITION;   //顶点坐标
};

// 输出结构体
struct VS_OUTPUT
{
    float4 position : SV_POSITION; // 系统值语义表示顶点位置
    float3 viewRay : POSITION0;
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

cbuffer MVPProjectCB : register(b1) 
{
    float4x4 model_from_view; // inverses of viewMatrix
    float4x4 view_from_clip;  // inverses of projectMatrix
};

struct AtmosphereRenderParameters
{
	[[vk::offset(0)]] float3 camera;   // 相机的位置
  	[[vk::offset(12)]] float exposure;  // 曝光度
    [[vk::offset(16)]] float3 white_point; //
    [[vk::offset(32)]] float3 earth_center; // 地球中心点
    [[vk::offset(48)]] float3 sun_direction; // 太阳方向
    [[vk::offset(64)]] float2 sun_size; // 太阳圆盘大小
};

cbuffer AtmosphereRenderParametersCB : register(b2)
{
    AtmosphereRenderParameters RenderParameters;
};


[shader("vertex")]
VS_OUTPUT VS(VS_INPUT vin)
{
    VS_OUTPUT output;
    output.position = float4(vin.PosL, 1.0f);

	float4 pos = mul(float4(output.position.xyz, 1.0), view_from_clip);
    pos = mul(pos, model_from_view);

    output.viewRay = pos.xyz;
    return output;
}

const float3 kSphereCenter = float3(0.0, 0.0, 1000.0) / 1.0; //kLengthUnitInMeters;
const float kSphereRadius = 1000.0 / 1.0;//kLengthUnitInMeters;
const float3 kSphereAlbedo = float3(0.8, 0.8, 0.8);
const float3 kGroundAlbedo = float3(0.0, 0.0, 0.04);

float GetSunVisibility(float3 pos, float3 sun_direction)
{
	float3	p	= pos - kSphereCenter;
	float	p_dot_v   = dot(p, sun_direction);
	float	p_dot_p	= dot(p, p);
	// 球体中心到视线的垂直距离的平方
	float	ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
	float	distance_to_intersection = -p_dot_v - sqrt(kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance);
	if (distance_to_intersection > 0.0)
    {
		float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_distance);
		float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
		return smoothstep(1.0, 0.0, ray_sphere_angular_distance / RenderParameters.sun_size.x);
	}
	return 1.0;
}

// 计算在给定pos位置，有多少比例的天空光没有被球体遮挡
float GetSkyVisibility(float3 pos) 
{
    float3 p = pos - kSphereCenter;
    float p_dot_p = dot(p, p);
    return 1.0 + p.z / sqrt(p_dot_p) * kSphereRadius * kSphereRadius / p_dot_p;
}

void GetSphereShadowInOut(float3 view_direction, float3 sun_direction, out float d_in, out float d_out)
{
	float3	pos		= RenderParameters.camera - kSphereCenter;
	float	pos_dot_sun	= dot(pos, sun_direction);
	float	view_dot_sun	= dot(view_direction, sun_direction);
	float	k		= RenderParameters.sun_size.x;
	float	l		= 1.0 + k * k;
	float	a		= 1.0 - l * view_dot_sun * view_dot_sun;
	float	b		= dot(pos, view_direction) - l * pos_dot_sun * view_dot_sun -
				  			k * kSphereRadius * view_dot_sun;
	float c 		= dot(pos, pos) - l * pos_dot_sun * pos_dot_sun -
		  2.0 * k * kSphereRadius * pos_dot_sun - kSphereRadius * kSphereRadius;
	float discriminant = b * b - a * c;
	if (discriminant > 0.0)
    {
		d_in	= max(0.0, (-b - sqrt(discriminant)) / a);
		d_out	= (-b + sqrt(discriminant)) / a;
		/* The values of d for which delta is equal to 0 and kSphereRadius / k. */
		float	d_base	= -pos_dot_sun / view_dot_sun;
		float	d_apex	= -(pos_dot_sun + kSphereRadius / k) / view_dot_sun;
		if (view_dot_sun > 0.0)
        {
			d_in	= max(d_in, d_apex);
			d_out	= a > 0.0 ? min(d_out, d_base) : d_base;
		} 
        else 
        {
			d_in	= a > 0.0 ? max(d_in, d_base) : d_base;
			d_out	= min(d_out, d_apex);
		}
	} 
    else 
    {
		d_in	= 0.0;
		d_out	= 0.0;
	}
}

Texture2D transmittance_texture;
Texture3D scattering_texture;
Texture3D single_mie_scattering_texture;
Texture2D irradiance_texture;

static const SamplerState linear_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
    MaxAnisotropy = 1;
    ComparisonFunc = NEVER;
    MinLOD = 0;
    MaxLOD = FLOAT32_MAX;
};

float3 GetSolarRadiance() 
{
	return ATMOSPHERE.solar_irradiance /
      		(PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
}

float3 GetSkyRadiance(
	float3 camera, float3 view_ray, float shadow_length,
	float3 sun_direction, out float3 transmittance) 
{
	return GetSkyRadiance(ATMOSPHERE, transmittance_texture, linear_sampler,
      		scattering_texture, linear_sampler, single_mie_scattering_texture, linear_sampler,
      		camera, view_ray, shadow_length, sun_direction, transmittance);
}

float3 GetSkyRadianceToPoint(
	float3 camera, float3 pos, float shadow_length,
	float3 sun_direction, out float3 transmittance) 
{
	return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture, linear_sampler,
      		scattering_texture, linear_sampler, single_mie_scattering_texture, linear_sampler,
      		camera, pos, shadow_length, sun_direction, transmittance);
}

float3 GetSunAndSkyIrradiance(
	float3 p, float3 normal, float3 sun_direction,
	out float3 sky_irradiance) 
{
	return GetSunAndSkyIrradiance(ATMOSPHERE, transmittance_texture, linear_sampler,
      		irradiance_texture, linear_sampler, p, normal, sun_direction, sky_irradiance);
}

[shader("pixel")]
float4 PS(VS_OUTPUT vs_output) : SV_Target0 
{
    // Normalized view direction vector.
    float3 view_ray = vs_output.viewRay;
    float3 view_direction = normalize(view_ray);
    // Tangent of the angle subtended by this fragment.
    float fragment_angular_size = length(ddx(view_ray) + ddy(view_ray)) / length(view_ray);

    float shadow_in;
    float shadow_out;
    GetSphereShadowInOut(view_direction, RenderParameters.sun_direction, shadow_in, shadow_out);

    float3 camera = RenderParameters.camera;
    // Hack to fade out light shafts when the Sun is very close to the horizon.
    float lightshaft_fadein_hack = smoothstep(
        0.02, 0.04, dot(normalize(camera - RenderParameters.earth_center), RenderParameters.sun_direction));

	// Compute the distance between the view ray line and the sphere center,
	// and the distance between the camera and the intersection of the view
	// ray with the sphere (or NaN if there is no intersection).
	// 计算视线与球体中心的距离以及交点到相机的距离
	float3 p = camera - kSphereCenter;
	float p_dot_v = dot(p, view_direction);
	float p_dot_p = dot(p, p);
	float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
	float discriminant =
		kSphereRadius * kSphereRadius -ray_sphere_center_squared_distance;

	// Compute the radiance reflected by the sphere, if the ray intersects it.
	float sphere_alpha = 0.0;
	float3 sphere_radiance = float3(0.0, 0.0, 0.0);
	if (discriminant >= 0.0) 
	{
		// 视线到球体边界的距离
		float distance_to_intersection = -p_dot_v - sqrt(discriminant);
		if (distance_to_intersection > 0.0) 
		{
			// Compute the distance between the view ray and the sphere, and the
			// corresponding (tangent of the) subtended angle. Finally, use this to
			// compute the approximate analytic antialiasing factor sphere_alpha.
			float ray_sphere_distance =
				kSphereRadius - sqrt(ray_sphere_center_squared_distance);
			float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
			sphere_alpha =
				min(ray_sphere_angular_distance / fragment_angular_size, 1.0);

			float3 pos = camera + view_direction * distance_to_intersection;
			float3 normal = normalize(pos - kSphereCenter);

			// Compute the radiance reflected by the sphere.
			float3 sky_irradiance;
			float3 sun_irradiance = GetSunAndSkyIrradiance(
				pos - RenderParameters.earth_center, normal, RenderParameters.sun_direction, sky_irradiance);
			sphere_radiance =
				kSphereAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);

			float shadow_length = max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) * lightshaft_fadein_hack;
			float3 transmittance;
			float3 in_scatter = GetSkyRadianceToPoint(camera - RenderParameters.earth_center,
				pos - RenderParameters.earth_center, shadow_length, RenderParameters.sun_direction, transmittance);
			sphere_radiance = sphere_radiance * transmittance + in_scatter;
		}
	}

	// Compute the distance between the view ray line and the Earth center,
	// and the distance between the camera and the intersection of the view
	// ray with the ground (or NaN if there is no intersection).
	p = camera - RenderParameters.earth_center;
	p_dot_v = dot(p, view_direction);
	p_dot_p = dot(p, p);
	float ray_earth_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
	discriminant = RenderParameters.earth_center.z * RenderParameters.earth_center.z - 
		ray_earth_center_squared_distance;

	// 计算视线到地球的距离、交点
	float ground_alpha = 0.0;
	float3 ground_radiance = float3(0.0, 0.0, 0.0);
	if (discriminant >= 0.0) 
	{
		float distance_to_intersection = -p_dot_v - sqrt(discriminant);
		if (distance_to_intersection > 0.0) 
		{
			// 交点
			float3 pos = camera + view_direction * distance_to_intersection;
			// 交点的法线
			float3 normal = normalize(pos - RenderParameters.earth_center);

			// Compute the radiance reflected by the ground.
			float3 sky_irradiance;
			float3 sun_irradiance = GetSunAndSkyIrradiance(
				pos - RenderParameters.earth_center, normal, RenderParameters.sun_direction, sky_irradiance);
			ground_radiance = kGroundAlbedo * (1.0 / PI) * (
				sun_irradiance * GetSunVisibility(pos, RenderParameters.sun_direction) +
				sky_irradiance * GetSkyVisibility(pos));

			float shadow_length =
				max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
				lightshaft_fadein_hack;
			float3 transmittance;
			float3 in_scatter = GetSkyRadianceToPoint(camera - RenderParameters.earth_center,
				pos - RenderParameters.earth_center, shadow_length, RenderParameters.sun_direction, transmittance);
			ground_radiance = ground_radiance * transmittance + in_scatter;
			ground_alpha = 1.0;
		}
	}

	// Compute the radiance of the sky.
	float shadow_length = max(0.0, shadow_out - shadow_in) *
		lightshaft_fadein_hack;
	float3 transmittance;
	float3 radiance = GetSkyRadiance(
		camera - RenderParameters.earth_center, view_direction, shadow_length, RenderParameters.sun_direction,
		transmittance);

	// If the view ray intersects the Sun, add the Sun radiance.
	if (dot(view_direction, RenderParameters.sun_direction) > RenderParameters.sun_size.y) 
	{
		radiance = radiance + transmittance * GetSolarRadiance();
	}
	radiance = lerp(radiance, ground_radiance, ground_alpha);
	radiance = lerp(radiance, sphere_radiance, sphere_alpha);
	float3 color = 
		pow(float3(1.0, 1.0, 1.0) - exp(-radiance / RenderParameters.white_point * RenderParameters.exposure), float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1.0);
}