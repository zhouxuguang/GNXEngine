#include "../GNXEngineCommon.hlsl"

struct v2f
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

v2f VS(uint vertexID : SV_VertexID)
{
    v2f vout;

    vout.position = fsTrianglePosition(vertexID);  
    vout.texcoord = fsTriangleUV(vertexID);  
    return vout;
}

Texture2D texImage;
SamplerState texImageSam;

float4 PS(v2f pin) : SV_Target
{
    float4 texColor = texImage.Sample(texImageSam, pin.texcoord);
    return texColor;
}