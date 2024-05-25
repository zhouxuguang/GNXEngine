#include "StandardBRDF.hlsl"
#include "Lighting.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;  //顶点输出坐标，裁剪空间
    float3 position : POSITION;   // 顶点坐标，输出到世界坐标，为后续光源计算做准备
    float3 normal : NORMAL;
    float4 tangent : TANGENT;    
    float2 texCoord0 : TEXCOORD0;     // 纹理坐标
    float3 lightDir : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
    float3 worldNormal : TEXCOORD3;   // 世界坐标法线，为光源计算准备
};

VertexOut VS(appdata_skin vin)
{
    VertexOut vout;
    
    // Transform to world space.
    float4 posW = mul(float4(vin.position, 1.0), MATRIX_M);
    vout.position = posW;

    posW = mul(posW, MATRIX_V);
    posW = mul(posW, MATRIX_P);
    vout.PosH = posW;
    vout.texCoord0 = vin.texcoord;

    // 法线转换到视线空间
    float3 norm = normalize(mul(vin.normal, MATRIX_Normal));
    vout.worldNormal = norm;
    norm = normalize(mul(norm, MATRIX_V));
    vout.normal = norm;

    // 切线方向转换到视线空间
    float3 tang = normalize(mul(vin.tangent, MATRIX_Normal));
    tang = normalize(mul(tang, MATRIX_V));
    vout.tangent = float4(tang, vin.tangent.w);

    // 将视线方向和光源方向转换到视线空间
    float4 pos = mul(float4(vin.position, 1.0), MATRIX_M);   // 世界坐标的顶点 

    float3 lightPos = _WorldSpaceLightPos;
    vout.lightDir = mul(float4(lightPos - pos.xyz, 1.0), MATRIX_V).xyz;

    // 根据相机的位置计算相机的方向
    // float4 cameraPos = mul(float4(_WorldSpaceCameraPos, 1.0), MATRIX_V);
    pos = mul(pos, MATRIX_V);
    vout.viewDir = (-pos.xyz);
    
    return vout;
}


// 纹理和采样器
Texture2D gDiffuseMap : register(t0);
SamplerState gDiffuseSamp  : register(s0);

Texture2D gNormalMap : register(t1);
SamplerState gNormalSamp  : register(s1);

Texture2D gMetalRoughMap : register(t2);
SamplerState gMetalRoughSamp  : register(s2);

Texture2D gEmissiveMap : register(t3);
SamplerState gEmissiveSamp  : register(s3);

Texture2D gAmbientMap : register(t4);
SamplerState gAmbientSamp  : register(s4);

float4 PS(VertexOut pin) : SV_Target
{
    float4 Kao = gAmbientMap.Sample(gAmbientSamp, pin.texCoord0);
	float4 Ke  = gEmissiveMap.Sample(gEmissiveSamp, pin.texCoord0);
	float4 Kd  = gDiffuseMap.Sample(gDiffuseSamp, pin.texCoord0);
	//float2 MeR = gMetalRoughMap.Sample(gMetalRoughSamp, pin.texCoord0).xy;

	// world-space normal
	float3 n = normalize(pin.normal);

	float3 normalSample = gNormalMap.Sample(gNormalSamp, pin.texCoord0).xyz;

	// normal mapping
	n = perturbNormal(n, normalize(_WorldSpaceCameraPos - pin.position), normalSample, pin.texCoord0);

	float4 mrSample = gMetalRoughMap.Sample(gMetalRoughSamp, pin.texCoord0);

	PBRInfo pbrInputs;
	//Ke.rgb = SRGBtoLINEAR(Ke).rgb;
	// image-based lighting
	float3 color = calculatePBRInputsMetallicRoughness(Kd, n, _WorldSpaceCameraPos, pin.position, mrSample, pbrInputs);
	// one hardcoded light source
    color += calculatePBRLightContribution(pbrInputs, normalize(_WorldSpaceLightPos.xyz), _LightColor.xyz);
	// ambient occlusion
	color = color * (Kao.r < 0.01 ? 1.0 : Kao.r);
	// emissive
	color = Ke.rgb + color;

	return float4(color, 1.0);
}