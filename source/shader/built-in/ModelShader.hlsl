#include "GNXEngineCommon.hlsl"
#include "StandardBRDF.hlsl"
#include "Lighting.hlsl"

//模型坐标输出定义

// 灯光信息
cbuffer LightInfo : register(b2)
{
    float4 Position;  // Light position in world space.
    float3 Intensity; // A,D,S intensity
};

//材质信息
cbuffer MaterialInfo  : register(b3)
{
    float4 diffuseColor;  // 漫反射颜色
    float3 Ka;            // 环境光反射系数
    float3 Ks;            // 高光反射系数
    float Shininess;      // 高光系数
};

struct VertexOut
{
    float4 PosH : SV_POSITION;  //顶点输出坐标，裁剪空间
    float3 normal : NORMAL;
    float2 texCoord0 : TEXCOORD0;     // 纹理坐标
    float3 lightDir : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
};

float3 phongModel(float3 lightDir, float3 viewDir, float3 norm, float3 diffR) 
{
    float3 r = reflect(-lightDir, norm);
    float3 ambient = Intensity * Ka;
    float sDotN = max(dot(lightDir, norm), 0.0);
    float3 diffuse = Intensity * diffR * sDotN;

    float3 spec = float3(0.0, 0.0, 0.0);
    if( sDotN > 0.0 )
        spec = Intensity * Ks *
               pow(max(dot(r, viewDir), 0.0), Shininess);

    return ambient + diffuse + spec;
}

VertexOut VS(appdata_tan vin)
{
    VertexOut vout;
    
    // Transform to world space.
    float4 posW = mul(float4(vin.position.xyz, 1.0), MATRIX_M);
    posW = mul(posW, MATRIX_V);
    posW = mul(posW, MATRIX_P);
    vout.PosH = posW;
    vout.texCoord0 = vin.texcoord;

    // 法线转换到视线空间
    float3 norm = normalize(mul(vin.normal.xyz, MATRIX_Normal));
    norm = normalize(mul(norm, MATRIX_V));
    vout.normal = norm;

    // 将视线方向和光源方向转换到视线空间
    float4 pos = mul(float4(vin.position.xyz, 1.0), MATRIX_M);
    pos = mul(pos, MATRIX_V);
    vout.lightDir = normalize((Position.xyz - pos));

    // 相机的位置
    float3 cameraPos = mul(float4(_WorldSpaceCameraPos, 1.0), MATRIX_V);
    vout.viewDir = normalize(cameraPos - pos.xyz);
    
    return vout;
}

Texture2D gDiffuseMap : register(t0);
SamplerState gSamLinearWrap  : register(s0);

float4 PS(VertexOut pin) : SV_Target
{
    float3 diffuseColor = gDiffuseMap.Sample(gSamLinearWrap, pin.texCoord0).xyz;
    // return float4(phongModel(pin.lightDir, pin.viewDir, pin.normal, diffuseColor), 1.0);
    return gDiffuseMap.Sample(gSamLinearWrap, pin.texCoord0);
}
