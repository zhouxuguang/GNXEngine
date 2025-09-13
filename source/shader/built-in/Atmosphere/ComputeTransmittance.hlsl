#include “AtmosphereDefine.hlsl”

// 输入结构体
struct VS_INPUT
{
    float2 vertex : POSITION; // 使用语义绑定顶点位置
};

// 输出结构体
struct VS_OUTPUT
{
    float4 position : SV_POSITION; // 系统值语义表示顶点位置
};

[shader("vertex")]
VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.vertex, 0.0, 1.0);
    return output;
}

struct PSOutput
{
    float3 transmittance : SV_Target0; // 对应 location = 0
};

cbuffer AtmosphereCB : register(b0)
{
    //AtmosphereParameters ATMOSPHERE;
};

[shader("pixel")]
PSOutput PSMain(float4 position : SV_Position)
{
    PSOutput output;
    
    // 获取当前像素坐标 (等效于 GLSL 的 gl_FragCoord.xy)
    float2 frag_coord = position.xy;
    
    // 计算透射率
    // output.transmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(
    //     ATMOSPHERE, frag_coord);
    
    return output;
}