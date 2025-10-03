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
    float4 scattering_density : SV_Target0; // 对应 location = 0
};

Texture2D transmittance_texture : register(t0);
Texture3D single_rayleigh_scattering_texture : register(t1);
Texture3D single_mie_scattering_texture : register(t2);
Texture3D multiple_scattering_texture : register(t3);
Texture2D irradiance_texture : register(t4);

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
    
    // 计算散射密度
    float3 scattering_density = ComputeScatteringDensityTexture(
        ATMOSPHERE,
        transmittance_texture,
        linear_sampler,
        single_rayleigh_scattering_texture,
        linear_sampler,
        single_mie_scattering_texture,
        linear_sampler,
        multiple_scattering_texture,
        linear_sampler,
        irradiance_texture,
        linear_sampler,
        frag_coord,
        scattering_order
    );

    output.scattering_density = float4(scattering_density, 1.0);
    
    return output;
}