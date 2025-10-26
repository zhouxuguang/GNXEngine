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

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

cbuffer ScatteringCB : register(b1) 
{
    int layer;  // 当前散射层
    int scattering_order;
};

// 输出结构体
struct PSOutput 
{
    float4 delta_multiple_scattering : SV_Target0; // 对应 location = 0
    float4 scattering : SV_TARGET1;
};

Texture2D transmittance_texture;
Texture3D scattering_density_texture;

static const SamplerState linear_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
    MaxAnisotropy = 1;
    ComparisonFunc = NEVER;
    MinLOD = 0;
    MaxLOD = FLOAT32_MAX;
};

[shader("pixel")]
PSOutput PS(float4 position : SV_Position)
{
    PSOutput output;
    
    // 计算片段坐标 (x, y, layer + 0.5)
    float3 frag_coord = float3(position.xy, layer + 0.5f);
    
    float nu = 0.0;
	output.delta_multiple_scattering.rgb = ComputeMultipleScatteringTexture(
      		ATMOSPHERE, transmittance_texture, linear_sampler, scattering_density_texture, linear_sampler,
      		frag_coord, nu);
    output.delta_multiple_scattering.a = 1.0;
	output.scattering = float4(output.delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);
    
    return output;
}