// Screen-space reflection composition pass.

#include "GNXEngineCommon.hlsl"
#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut VS(uint vertexID : SV_VertexID)
{
    VertexOut output;
    output.PosH = fsTrianglePosition(vertexID);
    output.texCoord = fsTriangleUV(vertexID);
    return output;
}

Texture2D gSceneColor;
SamplerState gSceneColorSam;
Texture2D gReflection;
SamplerState gReflectionSam;
Texture2D gGBufferA;
SamplerState gGBufferASam;
Texture2D gGBufferB;
SamplerState gGBufferBSam;
Texture2D gGBufferC;
SamplerState gGBufferCSam;
Texture2D gDepth;
SamplerState gDepthSam;

cbuffer cbSSRParams
{
    float4 TraceParams;
    float4 FadeParams;
}

float4 PS(VertexOut input) : SV_Target0
{
    float2 uv = input.texCoord;
    float3 scene = gSceneColor.Sample(gSceneColorSam, uv).rgb;
    float depth = gDepth.Sample(gDepthSam, uv).r;
#ifdef USE_REVERSE_Z
    if (depth <= 0.00001)
        return float4(scene, 1.0);
#else
    if (depth >= 0.99999)
        return float4(scene, 1.0);
#endif

    float4 reflection = gReflection.Sample(gReflectionSam, uv);
    if (reflection.a <= 0.0001)
        return float4(scene, 1.0);

    float4 normalRoughness = gGBufferA.Sample(gGBufferASam, uv);
    float metallic = saturate(gGBufferB.Sample(gGBufferBSam, uv).r);
    float3 albedo = gGBufferC.Sample(gGBufferCSam, uv).rgb;
    float3 normal = DecodeNormalOctahedron(normalRoughness.xyz);
    float3 worldPosition = ReconstructWorldPosition(uv, depth);
    float3 viewDirection = normalize(_WorldSpaceCameraPos - worldPosition);
    float nDotV = saturate(dot(normal, viewDirection));
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 fresnel = f0 + (1.0 - f0) * pow(1.0 - nDotV, 5.0);

    float roughness = saturate(gGBufferB.Sample(gGBufferBSam, uv).b);
    float smoothness = 1.0 - roughness;
    float dielectricBoost = lerp(0.45, 1.0, metallic);
    float weight = reflection.a * smoothness * smoothness * dielectricBoost * FadeParams.z;
    float3 reflected = reflection.rgb * max(fresnel, 0.12);
    return float4(scene + reflected * weight, 1.0);
}
