// Render 16 32x32 slices into a 3D RGBA16F texture. NewSkyLutCB.x is the
// currently bound slice, y is range in km (96), z is the slice count (16).
#define NEW_SKY_USE_VIEW 1
#define NEW_SKY_USE_PLANET 1
#define NEW_SKY_USE_PASS 1
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
    float sliceCount = max(new_sky_lut_params.z, 1.0);
    float w = (new_sky_lut_params.x + 0.5) / sliceCount;
    float distanceKm = w * w * NewSkyAerialRangeKm();
    float3 ray = NewSkyScreenRay(input.uv);
    // UE 5.3: slice k marches (k+1)*2 fixed linear samples from the camera.
    float sampleCount = (new_sky_lut_params.x + 1.0) * NEW_SKY_AERIAL_SAMPLES_PER_SLICE;
    NewSkyIntegration result = NewSkyIntegrate(NewSkyProxyCameraKm(), ray,
        normalize(sun_direction_pad.xyz), distanceKm, sampleCount,
        false, true, false);
    float transmittance = dot(result.transmittance,
        float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));
    return float4(result.luminance * ATMOSPHERE.solar_irradiance,
                  transmittance);
}
