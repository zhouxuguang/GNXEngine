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

VertexOut VS(appdata_tan vin)
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
#if 0
    float3 diffuseColor = gDiffuseMap.Sample(gDiffuseSamp, pin.texCoord0).xyz;
    float3 normal = gNormalMap.Sample(gNormalSamp, pin.texCoord0).xyz;
    
	float3 bumpedNormalW = NormalSampleToWorldSpace(normal, pin.normal, pin.tangent);

    // 注释下面这行取消法线贴图
	//bumpedNormalW = pin.normal;

    float2 metalRough = gMetalRoughMap.Sample(gMetalRoughSamp, pin.texCoord0).xy;
    float metallic = metalRough.x;
    float roughness = metalRough.y;
    float3 emissive = gEmissiveMap.Sample(gEmissiveSamp, pin.texCoord0).xyz;

    float ao = gAmbientMap.Sample(gAmbientSamp, pin.texCoord0).r;

    // 法线
    float3 N = normalize(bumpedNormalW);
    //N = normalize(pin.normal);

    // 视线方向
    float3 V = normalize(pin.viewDir);

    // 光源方向
    float3 L = normalize(pin.lightDir);

    float3 h = normalize( V + L );
    float nDotH = saturate(dot(N, h));
    float lDotH = saturate(dot(L, h));
    float nDotL = saturate(dot(N, L));
    float nDotV = saturate(dot(N, V));

    //确定f0
    float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, diffuseColor, metallic);

    // cook-torrance brdf
    float NDF = DistributionGGX(nDotH, roughness);        
    float G   = GeometrySchlickGGX(nDotL, roughness) * GeometrySchlickGGX(nDotV, roughness);      
    float3 F    = FresnelTerm(F0, lDotH);  

    // 确定kd和ks
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic; 
    diffuseColor = (1.0 - metallic) * diffuseColor;

    // 确定高光项
    float3 nominator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; 
    float3 specular     = nominator / denominator;

    // float3 reflectDir = normalize(reflect(-viewDir, N)); //反射方向
	// float3 reflection = UNITY_SAMPLE_TEXCUBE_LOD(unity_SpecCube0, reflectDir, roughness * 5).rgb;//反射目标

    // specular += lerp(specular, reflection, F);

    // 光源相关也写死 
    Light pointLight = CreateLightInfo();
    float3 radiance = ComputePointLight(pointLight, pin.position, normalize(pin.worldNormal)); 

    // 确定最后输出的能量 
    float diffuseCoff = DisneyDiffuse(nDotV, nDotL, lDotH, roughness);             
    float3 Lo = (diffuseCoff * diffuseColor / UNITY_PI + specular) * radiance * nDotL; 

    float3 ambient = float3(0.03, 0.03, 0.03) * diffuseColor * ao;
    float3 color = Lo + emissive + ambient;
    //color = color / (color + float3(1.0, 1.0, 1.0));

    //return float4(roughness, roughness, roughness, 1.0);
    return float4(color, 1.0);
    //pin.texCoord0.y = 1.0 - pin.texCoord0.y;
    //return gDiffuseMap.Sample(gDiffuseSamp, pin.texCoord0);

#else
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
#endif
}