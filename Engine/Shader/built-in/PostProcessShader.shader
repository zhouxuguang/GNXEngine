//
//  PostProcessShader.shader
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/6.
//

#include "GNXEngineCommon.hlsl"
#include "ToneMapping.hlsl"

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

//[[vk::combinedImageSampler]]
Texture2D texImage;// : register(t0, space0);
//[[vk::combinedImageSampler]]
SamplerState texImageSam;//  : register(s1, space0);

half4 PS(v2f pin) : SV_Target
{
    float4 texColor = texImage.Sample(texImageSam, pin.texcoord);

    return half4(TonemapSceneColor(texColor), 1.0h);
}
