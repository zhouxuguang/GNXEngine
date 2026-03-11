#ifndef GNX_ENGINE_BASEPASS_HLSL
#define GNX_ENGINE_BASEPASS_HLSL

#include "GNXEngineVariables.hlsl"

// 顶点着色器输入
struct VertexInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    // lightSpaceMatrix * model * position
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    float3 normal = mul(float4(input.normal.xyz, 0.0f), MATRIX_Normal).xyz;
    output.normal = normal;
    
    return output;
}

struct FragmentOutput 
{
    float4 outRT0 : SV_TARGET0;
    float4 outRT1 : SV_TARGET1;
    float4 outRT2 : SV_TARGET2;
    float4 outRT3 : SV_TARGET3;
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);
    normal = normal * 0.5 + 0.5;
    
    FragmentOutput output;

    output.outRT0 = float4(input.position.xyz, 1.0f);

    output.outRT1 = float4(normal, 0.333333f);

    output.outRT2 = float4(0.0f, 0.0f, 1.0f, 1.0f);

    output.outRT3 = float4(0.0f, 0.0f, 1.0f, 1.0f);

    return output;
}

#endif