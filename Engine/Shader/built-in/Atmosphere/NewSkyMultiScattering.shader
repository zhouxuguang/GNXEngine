// 32 x 32 RGBA16F. x = sun zenith cosine, y = normalized altitude.
// Stores the second and all higher scattering orders per unit solar irradiance.
#define NEW_SKY_USE_TRANSMITTANCE 1
#include "NewSkyAtmosphereCommon.hlsl"

#ifndef NEW_SKY_MULTI_DIRECTION_COUNT
#define NEW_SKY_MULTI_DIRECTION_COUNT 8
#endif

[shader("vertex")]
NewSkyVertexOutput VS(uint vertexID : SV_VertexID)
{
    return NewSkyFullscreenVertex(vertexID);
}

[shader("pixel")]
float4 PS(NewSkyVertexOutput input) : SV_Target0
{
    float2 uv = input.position.xy / float2(NEW_SKY_MULTI_WIDTH, NEW_SKY_MULTI_HEIGHT);
    float muSun = uv.x * 2.0 - 1.0;
    float3 sun = float3(sqrt(max(1.0 - muSun * muSun, 0.0)), 0.0, muSun);
    float radiusKm = lerp(NewSkyBottomKm() + 0.001, NewSkyTopKm() - 0.001, uv.y);
    float3 originKm = float3(0.0, 0.0, radiusKm);
    float3 secondOrder = float3(0.0, 0.0, 0.0);
    float3 uniformTransfer = float3(0.0, 0.0, 0.0);
    [loop]
    for (int i = 0; i < NEW_SKY_MULTI_DIRECTION_COUNT; ++i)
    {
        // Equal-area sphere directions. UE's default fast mode uses two
        // directions; eight reduce horizon bias while keeping LUT updates cheap.
        float z = 1.0 - 2.0 * (float(i) + 0.5) /
            float(NEW_SKY_MULTI_DIRECTION_COUNT);
        float azimuth = float(i) * 2.39996322972865332;
        float horizontal = sqrt(max(1.0 - z * z, 0.0));
        float3 ray = float3(horizontal * cos(azimuth),
                            horizontal * sin(azimuth), z);
        NewSkyIntegration sample = NewSkyIntegrate(originKm, ray, sun, -1.0,
            15, true, false, true);
        secondOrder += sample.luminance;
        uniformTransfer += sample.uniformTransfer;
    }
    secondOrder /= float(NEW_SKY_MULTI_DIRECTION_COUNT);
    uniformTransfer /= float(NEW_SKY_MULTI_DIRECTION_COUNT);
    // Equation 9/10 in Hillaire 2020: L2 / (1 - f_ms).
    float3 allOrders = secondOrder / max(1.0 - min(uniformTransfer, 0.999), 0.001);
    return float4(allOrders, 1.0);
}
