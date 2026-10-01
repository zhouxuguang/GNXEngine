#include "GNXEngineVariables.hlsl"
#include "GBufferCommon.hlsl"

Texture2D gDiffuseMap;
SamplerState gDiffuseMapSam;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0), MATRIX_M);
    output.position = mul(mul(worldPosition, MATRIX_V), MATRIX_P);
    output.normal = normalize(mul(float4(input.normal, 0.0), MATRIX_Normal).xyz);
    output.texCoord = input.texCoord;
    return output;
}

struct FragmentOutput
{
    float4 sceneColor : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 material : SV_TARGET2;
    float4 baseColor : SV_TARGET3;
    float4 motion : SV_TARGET4;
};

FragmentOutput PS(VertexOutput input)
{
    FragmentOutput output;
    float3 albedo = gDiffuseMap.Sample(gDiffuseMapSam, input.texCoord).rgb;
    output.sceneColor = float4(albedo, 1.0);
    output.normal = float4(EncodeNormalOctahedron(normalize(input.normal)), 0.333333);
    // RT2.a = 1 is reserved for unlit deferred geometry.
    output.material = float4(0.0, 0.5, 0.85, 1.0);
    output.baseColor = float4(albedo, 1.0);
    output.motion = float4(0.0, 0.0, 0.0, 0.0);
    return output;
}
