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
Texture2D gEmissiveMap;
SamplerState gEmissiveMapSam;

// 顶点着色器输入
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position    : SV_POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    float4 prevClipPos : TEXCOORD1;  // 上一帧的裁剪坐标（用于Motion Vector）
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    // 计算上一帧的裁剪坐标（用于 Motion Vector）
    // 当前假设物体是静态的（PrevM = M），动态物体需要传入 MATRIX_PrevM
    float4 prevWorldPos = worldPos;
    output.prevClipPos = mul(prevWorldPos, MATRIX_PrevVP);

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
    float4 outRT4 : SV_TARGET4;   //Motion Vector (NDC空间偏移，RG通道)
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
    float3 emissive = gEmissiveMap.Sample(gEmissiveMapSam, input.texCoord).rgb;
    
    FragmentOutput output;

    // RT0: Scene Color (写入自发光颜色，延迟渲染阶段会叠加光照)
    output.outRT0 = float4(emissive.rgb, 1.0f);

    // 法线编码
    normal = EncodeNormalOctahedron(normalize(normal));
    output.outRT1 = float4(normal, 0.333333f);

    // RT2: Metallic(0) + Specular(0.5) + Roughness(g通道) + [4 bit 0b1010 | 4 bit ShadingModel]
    uint lastCompoent = (10 << 4) | (1);
    output.outRT2 = float4(metalRough.r, 0.5f, metalRough.g, float(lastCompoent) / 255.0f);

    // RT3: BaseColor + AO
    output.outRT3 = float4(baseColor.rgb, ao);

    // RT4: Motion Vector（NDC 空间偏移）
    // 当前帧NDC减去上一帧NDC，存储为屏幕空间速度
    float2 motionVector = float2(0.0, 0.0);
    // 如果上一帧VP不是零矩阵（非第一帧），计算运动矢量
    if (input.prevClipPos.w != 0.0)
    {
        float2 curNDC = input.position.xy / input.position.w;
        float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
        motionVector = curNDC - prevNDC;
    }
    output.outRT4 = float4(motionVector, 0.0, 0.0);

    return output;
}

#endif