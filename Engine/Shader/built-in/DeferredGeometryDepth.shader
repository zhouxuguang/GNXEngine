#include "GNXEngineVariables.hlsl"

struct VertexInput
{
    float3 position : POSITION;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0), MATRIX_M);
    output.position = mul(mul(worldPosition, MATRIX_V), MATRIX_P);
    return output;
}

float PS(VertexOutput input) : SV_Depth
{
    return input.position.z;
}
