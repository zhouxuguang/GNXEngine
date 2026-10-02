// 256 x 64 RGBA16F, static until atmosphere material/radii change.
#include "NewSkyAtmosphereCommon.hlsl"

[shader("vertex")]
NewSkyVertexOutput VS(uint vertexID : SV_VertexID)
{
    return NewSkyFullscreenVertex(vertexID);
}

[shader("pixel")]
float4 PS(NewSkyVertexOutput input) : SV_Target0
{
    float2 uv = input.position.xy /
        float2(NEW_SKY_TRANSMITTANCE_WIDTH, NEW_SKY_TRANSMITTANCE_HEIGHT);
    float radiusKm, mu;
    NewSkyTransmittanceParams(uv, radiusKm, mu);
    float3 ray = float3(sqrt(max(1.0 - mu * mu, 0.0)), 0.0, mu);
    float3 originKm = float3(0.0, 0.0, radiusKm);
    float nearT, farT;
    NewSkyRaySphere(originKm, ray, NewSkyTopKm(), nearT, farT);
    // UE 5.3 r.SkyAtmosphere.TransmittanceLUT.SampleCount = 10.
    float stepKm = max(farT, 0.0) / 10.0;
    float3 opticalDepth = float3(0.0, 0.0, 0.0);
    [loop]
    for (int i = 0; i < 10; ++i)
    {
        float3 p = originKm + ray * (stepKm * (float(i) + NEW_SKY_SAMPLE_OFFSET));
        opticalDepth += NewSkySampleMedium(p).extinction * stepKm;
    }
    return float4(exp(-opticalDepth), 1.0);
}
