#include "AtmosphereCommon.hlsl"

// 输出结构体
struct VS_OUTPUT
{
    float4 position : SV_POSITION; // 系统值语义表示顶点位置
};

float4 fsTrianglePosition(uint vtx) 
{
  float x = -1.0 + float((vtx & 1) << 2);
  float y = -1.0 + float((vtx & 2) << 1);
  return float4(x, y, 0.0, 1.0);
}

[shader("vertex")]
VS_OUTPUT VS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    output.position = fsTrianglePosition(vertexID);
    return output;
}

struct PSOutput 
{
    float4 delta_irradiance : SV_Target0; // 对应 location = 0
    float4 irradiance       : SV_Target1; // 对应 location = 1
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

Texture2D transmittance_texture : register(t0);
SamplerState transmittance_sampler : register(s0);

[shader("pixel")]
PSOutput PS(float4 position : SV_Position)
{
    PSOutput output;
    
    // 计算直接辐照度增量
    output.delta_irradiance = float4(ComputeDirectIrradianceTexture(
        ATMOSPHERE, 
        transmittance_texture, 
        transmittance_sampler, 
        position.xy
    ), 1.0);
    
    // 初始化总辐照度为0
    output.irradiance = float4(0.0, 0.0, 0.0, 0.0);
    
    return output;
}