// Planet-aware atmosphere composition. Distances and extinction use meters.
#include "AtmosphereCommon.hlsl"

cbuffer AtmosphereParametersCB : register(b0) { AtmosphereParameters ATMOSPHERE; };
cbuffer AtmosphereViewCB : register(b1)
{
    float4x4 inv_view_proj;
    float4 camera_pos_exposure;
    float4 sun_direction_pad;
    float4 sun_size_pad;
    float4 white_point_pad;
};
cbuffer AtmospherePlanetCB : register(b2)
{
    float4 ground_radii_top_height;
    float4 camera_up_altitude;
};

Texture2D scene_color_texture;
SamplerState scene_color_textureSam;
Texture2D scene_depth_texture;
SamplerState scene_depth_textureSam;
Texture2D transmittance_texture;
SamplerState transmittance_textureSam;
Texture3D scattering_texture;
SamplerState scattering_textureSam;
Texture3D single_mie_scattering_texture;
SamplerState single_mie_scattering_textureSam;
Texture2D irradiance_texture;
SamplerState irradiance_textureSam;

struct VS_OUTPUT { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };

[shader("vertex")]
VS_OUTPUT VS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    output.position = fsTrianglePosition(vertexID);
    output.uv = fsTriangleUV(vertexID);
    return output;
}

float3 SkyRadiance(float3 camera, float3 ray, float3 sun, out float3 transmittance)
{
    return GetSkyRadiance(ATMOSPHERE, transmittance_texture, transmittance_textureSam,
        scattering_texture, scattering_textureSam,
        single_mie_scattering_texture, single_mie_scattering_textureSam,
        camera, ray, 0.0, sun, transmittance);
}

float3 SegmentRadiance(float3 camera, float3 endpoint, float3 sun, out float3 transmittance)
{
    return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture, transmittance_textureSam,
        scattering_texture, scattering_textureSam,
        single_mie_scattering_texture, single_mie_scattering_textureSam,
        camera, endpoint, 0.0, sun, transmittance);
}

float3 GroundIrradiance(float3 endpoint, float3 normal, float3 sun)
{
    float3 sky;
    float3 direct = GetSunAndSkyIrradiance(ATMOSPHERE,
        transmittance_texture, transmittance_textureSam,
        irradiance_texture, irradiance_textureSam,
        endpoint, normal, sun, sky);
    return ATMOSPHERE.ground_albedo * (direct + sky) / PI;
}

// Ray against the actual ellipsoid in planet-relative coordinates.
bool IntersectEllipsoid(float3 origin, float3 ray, float3 radii, out float distance)
{
    float3 o = origin / radii;
    float3 d = ray / radii;
    float a = dot(d, d);
    float b = dot(o, d);
    float c = dot(o, o) - 1.0;
    float disc = b * b - a * c;
    if (disc < 0.0)
    {
        distance = 0.0;
        return false;
    }
    float root = sqrt(max(disc, 0.0));
    float nearDistance = (-b - root) / a;
    float farDistance = (-b + root) / a;
    distance = nearDistance > 0.0 ? nearDistance : farDistance;
    return distance > 0.0;
}

// Closest point on a rotational ellipsoid; height is measured along its normal.
float GeodeticHeight(float3 endpoint, float3 radii)
{
    float3 radiiSquared = radii * radii;
    float shortest = min(radii.x, min(radii.y, radii.z));
    float lambda = (length(endpoint) - shortest) * shortest;
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        float3 q = max(radiiSquared + lambda, float3(1.0, 1.0, 1.0));
        float3 n = endpoint * endpoint * radiiSquared;
        float f = dot(n / (q * q), float3(1.0, 1.0, 1.0)) - 1.0;
        float df = -2.0 * dot(n / (q * q * q), float3(1.0, 1.0, 1.0));
        lambda -= f / min(df, -1e-20);
    }
    float3 surface = endpoint * radiiSquared / (radiiSquared + lambda);
    float3 normal = normalize(surface / radiiSquared);
    return dot(endpoint - surface, normal);
}

float3 ProxyRay(float3 worldRay, float3 up, float mu)
{
    float3 tangent = worldRay - dot(worldRay, up) * up;
    float tangentLength = length(tangent);
    if (tangentLength < 1e-5)
    {
        float3 axis = abs(up.z) < 0.9 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
        tangent = normalize(cross(up, axis));
    }
    else tangent /= tangentLength;
    return tangent * sqrt(max(1.0 - mu * mu, 0.0)) + up * mu;
}

// Match the endpoint's geodetic altitude at long ranges while preserving
// the camera-relative view direction for nearby terrain.
float ProxyCosine(float3 worldRay, float3 up, float cameraRadius, float endpointRadius, float distance)
{
    float directMu = dot(worldRay, up);
    if (distance < 1000.0) return directMu;
    float adjusted = ((endpointRadius - cameraRadius) * (endpointRadius + cameraRadius)
                    - distance * distance) / (2.0 * cameraRadius * distance);
    float blend = smoothstep(1000.0, 20000.0, distance);
    return clamp(lerp(directMu, clamp(adjusted, -1.0, 1.0), blend), -1.0, 1.0);
}

[shader("pixel")]
float4 PS(VS_OUTPUT input) : SV_Target0
{
    int2 pixel = int2(input.position.xy);
    float4 source = scene_color_texture.Load(int3(pixel, 0));
    float depth = scene_depth_texture.Load(int3(pixel, 0)).r;
#ifdef USE_REVERSE_Z
    bool hasSurface = depth > 0.0;
#else
    bool hasSurface = depth < 1.0;
#endif
    float2 uv = input.uv;
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    float4 clip = float4(uv * 2.0 - 1.0, 0.5, 1.0);
    float4 rayPoint = mul(clip, inv_view_proj);
    float3 worldRay = normalize(rayPoint.xyz / rayPoint.w);
    float3 camera = camera_pos_exposure.xyz;
    float3 sun = normalize(sun_direction_pad.xyz);
    float3 radii = ground_radii_top_height.xyz;
    float3 up = normalize(camera_up_altitude.xyz);
    float proxyCameraRadius = ATMOSPHERE.bottom_radius + max(camera_up_altitude.w, 0.0);
    float3 proxyCamera = up * proxyCameraRadius;
    float exposure = camera_pos_exposure.w;

    float ellipsoidDistance;
    bool hitsGround = IntersectEllipsoid(camera, worldRay, radii, ellipsoidDistance);
    float atmosphereDistance;
    bool hitsAtmosphere = IntersectEllipsoid(camera, worldRay,
        radii + ground_radii_top_height.w, atmosphereDistance);
    if (!hitsAtmosphere)
    {
        float3 sunDisk = dot(worldRay, sun) > sun_size_pad.y
            ? ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius)
            : float3(0.0, 0.0, 0.0);
        return float4(source.rgb + sunDisk * exposure, 1.0);
    }

    if (hasSurface || hitsGround)
    {
        float distance;
        float3 groundColor;
        float endpointHeight;
        if (hasSurface)
        {
            clip.z = depth;
            float4 offset = mul(clip, inv_view_proj);
            float3 cameraToSurface = offset.xyz / offset.w;
            distance = length(cameraToSurface);
            worldRay = cameraToSurface / max(distance, 1e-5);
            endpointHeight = max(GeodeticHeight(camera + cameraToSurface, radii), 0.0);
            groundColor = source.rgb;
        }
        else
        {
            distance = ellipsoidDistance;
            endpointHeight = 0.0;
            groundColor = float3(0.0, 0.0, 0.0);
        }
        float endpointRadius = ATMOSPHERE.bottom_radius + endpointHeight;
        float mu = ProxyCosine(worldRay, up, proxyCameraRadius, endpointRadius, distance);
        float3 proxyRay = ProxyRay(worldRay, up, mu);
        float3 proxyPoint = proxyCamera + proxyRay * distance;
        if (!hasSurface)
            groundColor = GroundIrradiance(proxyPoint, normalize(proxyPoint), sun);
        float3 transmittance;
        float3 inScatter = SegmentRadiance(proxyCamera, proxyPoint, sun, transmittance);
        return float4(groundColor * transmittance + inScatter * exposure, 1.0);
    }

    float mu = dot(worldRay, up);
    if (proxyCameraRadius > ATMOSPHERE.bottom_radius)
    {
        float tangentMu = -sqrt(max(1.0 - ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius /
                                      (proxyCameraRadius * proxyCameraRadius), 0.0));
        mu = max(mu, tangentMu + 1e-5);
    }
    float3 proxyRay = ProxyRay(worldRay, up, clamp(mu, -1.0, 1.0));
    float3 transmittance;
    float3 sky = SkyRadiance(proxyCamera, proxyRay, sun, transmittance);
    if (dot(worldRay, sun) > sun_size_pad.y)
        sky += transmittance * ATMOSPHERE.solar_irradiance /
            (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
    return float4(source.rgb + sky * exposure, 1.0);
}
