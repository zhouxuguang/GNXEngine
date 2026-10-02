// 192 x 104 RGBA16F; update when camera altitude, sun or atmosphere changes.
#define NEW_SKY_USE_VIEW 1
#define NEW_SKY_USE_PLANET 1
#define NEW_SKY_USE_TRANSMITTANCE 1
#define NEW_SKY_USE_MULTISCATTERING 1
#include "NewSkyAtmosphereCommon.hlsl"

[shader("vertex")]
NewSkyVertexOutput VS(uint vertexID : SV_VertexID)
{
    return NewSkyFullscreenVertex(vertexID);
}

[shader("pixel")]
float4 PS(NewSkyVertexOutput input) : SV_Target0
{
    float radiusKm = NewSkyProxyHeightKm();
    float2 uv = input.position.xy / float2(NEW_SKY_VIEW_WIDTH, NEW_SKY_VIEW_HEIGHT);
    float3 ray = NewSkySkyUvToLocalRay(uv, radiusKm);
    float sunMu = clamp(dot(NewSkyUp(), normalize(sun_direction_pad.xyz)), -1.0, 1.0);
    float3 sun = float3(sqrt(max(1.0 - sunMu * sunMu, 0.0)), 0.0, sunMu);
    NewSkyIntegration result = NewSkyIntegrate(float3(0.0, 0.0, radiusKm), ray,
        sun, -1.0, 0.0, false, true, false,
        NEW_SKY_SKYVIEW_SAMPLE_COUNT_MIN, NEW_SKY_SAMPLE_COUNT_MAX);
    return float4(result.luminance * ATMOSPHERE.solar_irradiance, 1.0);
}
