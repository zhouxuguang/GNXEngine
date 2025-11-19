
struct v2f
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 fsTrianglePosition(int vtx) 
{
  float x = -1.0 + float((vtx & 1) << 2);
  float y = -1.0 + float((vtx & 2) << 1);
  return float4(x, y, 0.0, 1.0);
}

float2 fsTriangleUV(int vtx) 
{
  float u = (vtx == 1) ? 2.0 : 0.0;
  float v = (vtx == 2) ? 2.0 : 0.0;
  return float2(u, v);
}

v2f VS(uint vertexID : SV_VertexID)
{
    v2f vout;

    vout.position = fsTrianglePosition(vertexID);  
    vout.texcoord = fsTriangleUV(vertexID);  
#ifdef TEXCOORD_FLIP
    vout.texcoord.y = 1.0 - vout.texcoord.y;
#endif
    return vout;
}

Texture2D texImage;
SamplerState texImageSam;

float4 PS(v2f pin) : SV_Target
{
    //float4 texColor = texImage.Sample(texImageSam, pin.texcoord);
    float4 texColor = float4(1.0, 0.0, 0.0, 1.0);
    return texColor;
}