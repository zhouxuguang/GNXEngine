#ifndef GNX_ENGINE_BASEPASS_HLSL
#define GNX_ENGINE_BASEPASS_HLSL

#include "GNXEngineVariables.hlsl"
#include "GBufferCommon.hlsl"

// 材质贴图
Texture2D gDiffuseMap;
SamplerState gDiffuseMapSam;
Texture2D gNormalMap;
SamplerState gNormalMapSam;
Texture2D gMetalRoughMap;
SamplerState gMetalRoughMapSam;
Texture2D gAmbientMap;
SamplerState gAmbientMapSam;

// 顶点着色器输入
struct VertexInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    // lightSpaceMatrix * model * position
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    float3 normal = mul(float4(input.normal.xyz, 0.0f), MATRIX_Normal).xyz;
    output.normal = normal;
    
    float3 tangent = mul(float4(input.tangent.xyz, 0.0f), MATRIX_Normal).xyz;
    output.tangent.xyz = tangent;
    output.tangent.w = input.tangent.w;
    
    output.texCoord = input.texCoord;
    
    return output;
}

struct FragmentOutput 
{
    float4 outRT0 : SV_TARGET0;   //scene color
    float4 outRT1 : SV_TARGET1;   //Normal + 0.33333f
    float4 outRT2 : SV_TARGET2;   //Metallic + Specular + Roughness + [4 bit 0b1010 | 4 bit ShadingModel]
    float4 outRT3 : SV_TARGET3;   //BaseColor + GenericAO
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);
    float4 tangent = float4(normalize(input.tangent.xyz), input.tangent.w);
    
    // 采样法线贴图（如果有）
    float3 normalTS = gNormalMap.Sample(gNormalMapSam, input.texCoord).xyz;
    normalTS = normalTS * 2.0 - 1.0;  // [0,1] -> [-1,1]
    normal = NormalSampleToWorldSpace(normalTS, normal, tangent);
    
    // 采样材质贴图
    float4 baseColor = gDiffuseMap.Sample(gDiffuseMapSam, input.texCoord);
    float4 metalRough = gMetalRoughMap.Sample(gMetalRoughMapSam, input.texCoord);
    float ao = gAmbientMap.Sample(gAmbientMapSam, input.texCoord).r;
    
    FragmentOutput output;

    output.outRT0 = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 法线编码
    normal = EncodeNormalOctahedron(normalize(normal));
    output.outRT1 = float4(normal, 0.333333f);

    // RT2: Metallic(0) + Specular(0.5) + Roughness(g通道) + [4 bit 0b1010 | 4 bit ShadingModel]
    uint lastCompoent = (10 << 4) | (1);
    output.outRT2 = float4(metalRough.r, 0.5f, metalRough.g, float(lastCompoent) / 255.0f);

    // RT3: BaseColor + AO
    output.outRT3 = float4(baseColor.rgb, ao);

    return output;
}

#endif