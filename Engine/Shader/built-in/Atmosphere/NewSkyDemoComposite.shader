// Standalone demo composition: the same analytic sphere and planet as the
// Bruneton demo, illuminated and viewed through the new sky atmosphere.
#define NEW_SKY_USE_VIEW 1
#define NEW_SKY_USE_PLANET 1
#define NEW_SKY_USE_TRANSMITTANCE 1
#define NEW_SKY_USE_MULTISCATTERING 1
#define NEW_SKY_USE_SKYVIEW_TEXTURE 1
#define NEW_SKY_USE_AERIAL_TEXTURE 1
#include "NewSkyAtmosphereCommon.hlsl"

cbuffer AtmosphereDemoCB : register(b4)
{
    float4 sphere_center_radius; // centre relative to planet (m), radius (m)
    float4 sphere_albedo_pad;
    float4 ground_albedo_pad;
};

Texture2D scene_color_texture;
SamplerState scene_color_textureSam;
Texture2D scene_depth_texture;
SamplerState scene_depth_textureSam;

[shader("vertex")]
NewSkyVertexOutput VS(uint vertexID : SV_VertexID)
{
    return NewSkyFullscreenVertex(vertexID);
}

float NewSkyDemoSphereDistance(float3 originM, float3 ray)
{
    float3 relative = originM - sphere_center_radius.xyz;
    float b = dot(relative, ray);
    float discriminant = b * b - dot(relative, relative)
        + sphere_center_radius.w * sphere_center_radius.w;
    if (discriminant < 0.0)
        return -1.0;
    float root = sqrt(max(discriminant, 0.0));
    float nearT = -b - root;
    float farT = -b + root;
    return nearT > 0.0 ? nearT : farT;
}

float NewSkyDemoSunVisibility(float3 pointM, float3 sun)
{
    float3 fromSphere = pointM - sphere_center_radius.xyz;
    float b = dot(fromSphere, sun);
    float discriminant = b * b - dot(fromSphere, fromSphere)
        + sphere_center_radius.w * sphere_center_radius.w;
    if (discriminant < 0.0)
        return 1.0;
    float nearT = -b - sqrt(max(discriminant, 0.0));
    return nearT > 0.0 ? 0.0 : 1.0;
}

[shader("pixel")]
float4 PS(NewSkyVertexOutput input) : SV_Target0
{
    float3 cameraM = camera_pos_exposure.xyz;
    float3 ray = NewSkyScreenRay(input.uv);
    float3 sun = normalize(sun_direction_pad.xyz);
    float exposure = camera_pos_exposure.w;

    float3 result = NewSkySkyRadiance(ray, sun) * exposure;
    float groundDistanceM;
    bool hitsGround = NewSkyIntersectEllipsoid(cameraM, ray,
        ground_radii_top_height.xyz, groundDistanceM);
    float sphereDistanceM = NewSkyDemoSphereDistance(cameraM, ray);

    if (hitsGround)
    {
        float distanceKm = groundDistanceM * 0.001;
        float3 up = NewSkyUp();
        float mu = NewSkyGroundProxyCosine(ray, up, distanceKm);
        float3 proxyRay = NewSkyProxyRay(ray, up, mu);
        float3 proxyGround = NewSkyProxyCameraKm() + proxyRay * distanceKm;
        float3 groundM = cameraM + ray * groundDistanceM;
        float3 normal = normalize(groundM /
            (ground_radii_top_height.xyz * ground_radii_top_height.xyz));
        float3 direct = ATMOSPHERE.solar_irradiance
            * NewSkySunTransmittance(proxyGround + normalize(proxyGround) * 0.001, sun)
            * max(dot(normal, sun), 0.0)
            * NewSkyDemoSunVisibility(groundM, sun);
        float3 ambient = NewSkyApproximateSkyIrradiance(sun);
        float3 surface = ground_albedo_pad.xyz * (direct + ambient) / PI;
        float4 aerial = NewSkyAerialForRay(input.uv, proxyRay,
            distanceKm, sun);
        result = (surface * aerial.a + aerial.rgb) * exposure;
    }

    if (sphereDistanceM > 0.0 &&
        (!hitsGround || sphereDistanceM < groundDistanceM))
    {
        float3 sphereM = cameraM + ray * sphereDistanceM;
        float3 normal = normalize(sphereM - sphere_center_radius.xyz);
        float3 proxySphereKm = NewSkyProxyCameraKm() + ray * (sphereDistanceM * 0.001);
        float3 surface = NewSkyLambertSurface(proxySphereKm, normal,
            sphere_albedo_pad.xyz, sun);
        float4 aerial = NewSkyAerialForRay(input.uv, ray,
            sphereDistanceM * 0.001, sun);
        float3 sphereColor = (surface * aerial.a + aerial.rgb) * exposure;

        // Analytic edge coverage, matching the reference demo's smooth sphere.
        float3 c = cameraM - sphere_center_radius.xyz;
        float projected = dot(c, ray);
        float closest2 = max(dot(c, c) - projected * projected, 0.0);
        float rayAngularSize = length(ddx(ray) + ddy(ray));
        float angularCoverage = sphere_center_radius.w
            - sqrt(closest2);
        float alpha = saturate(angularCoverage /
            max(-projected * rayAngularSize, 1e-5));
        result = lerp(result, sphereColor, alpha);
    }
    return float4(result, 1.0);
}
