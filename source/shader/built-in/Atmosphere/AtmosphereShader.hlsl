#include "AtmosphereCommon.hlsl"

struct VS_INPUT
{
    float3 PosL    : POSITION;   //顶点坐标
};

// 输出结构体
struct VS_OUTPUT
{
    float4 position : SV_POSITION; // 系统值语义表示顶点位置
    float3 viewRay : POSITION0;
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

cbuffer MVPProjectCB : register(b1) 
{
    float4x4 model_from_view; // inverses of viewMatrix
    float4x4 view_from_clip;  // inverses of projectMatrix
};

[shader("vertex")]
VS_OUTPUT VS(VS_INPUT vin)
{
    VS_OUTPUT output;
    output.position = float4(vin.PosL, 1.0f);
    output.viewRay = (model_from_view * float4((view_from_clip * vin.PosL).xyz, 0.0)).xyz;
    return output;
}
