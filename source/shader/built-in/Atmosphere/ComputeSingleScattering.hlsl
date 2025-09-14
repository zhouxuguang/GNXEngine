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

// 输出结构体定义
struct PSOutput 
{
    float4 delta_rayleigh : SV_Target0;        // location = 0
    float4 delta_mie       : SV_Target1;        // location = 1
    float4 scattering      : SV_Target2;        // location = 2
    float4 single_mie_scattering : SV_Target3;  // location = 3
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

cbuffer ScatteringCB : register(b1) 
{
    int layer;  // 当前散射层
};

struct GeometryInput 
{
    float4 position : SV_Position;
};

struct GeometryOutput 
{
    float4 position : SV_Position;
    uint layer : SV_RenderTargetArrayIndex; // 指定渲染目标数组索引
};

[maxvertexcount(3)] // 最大输出3个顶点
void GS(triangle GeometryInput input[3], inout TriangleStream<GeometryOutput> outputStream) 
{
    GeometryOutput output;
    
    output.position = input[0].position;
    output.layer = layer;
    outputStream.Append(output);
    
    output.position = input[1].position;
    output.layer = layer;
    outputStream.Append(output);
    
    output.position = input[2].position;
    output.layer = layer;
    outputStream.Append(output);
    
    outputStream.RestartStrip();
}

Texture2D transmittance_texture : register(t0);
SamplerState transmittance_sampler : register(s0);

[shader("pixel")]
PSOutput PS(float4 position : SV_Position)
{
    PSOutput output;

    // 计算单次散射
    float3 frag_coord = float3(position.xy, layer + 0.5f);
    float3 delta_rayleigh = float3(0.0, 0.0, 0.0);
    float3 delta_mie = float3(0.0, 0.0, 0.0);
    ComputeSingleScatteringTexture(
        ATMOSPHERE,
        transmittance_texture,
        transmittance_sampler,
        frag_coord,
        delta_rayleigh,
        delta_mie
    );
    
    // 组合散射结果
    output.scattering = float4(
        delta_rayleigh.rgb,  // RGB 通道存储 Rayleigh 散射
        delta_mie.r          // Alpha 通道存储 Mie 散射的 R 分量
    );

    output.delta_rayleigh = float4(delta_rayleigh, 1.0);
    output.delta_mie = float4(delta_mie, 1.0);
    
    // 存储完整的 Mie 散射
    output.single_mie_scattering = output.delta_mie;
    
    return output;
}