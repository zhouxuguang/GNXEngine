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
    float4 transmittance : SV_Target0; // 对应 location = 0
};

cbuffer AtmosphereParametersCB : register(b0)
{
    AtmosphereParameters ATMOSPHERE;
};

[shader("pixel")]
PSOutput PS(float4 position : SV_Position)
{
    PSOutput output;
    
    // 获取当前像素坐标 (等效于 GLSL 的 gl_FragCoord.xy)
    float2 frag_coord = position.xy;
    
    // 计算透射率
    float3 transmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(
        ATMOSPHERE, frag_coord);
    output.transmittance = float4(transmittance, 1.0);
    
    return output;
}