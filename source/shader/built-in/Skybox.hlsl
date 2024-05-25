//
//  Skybox.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/30.
//

#include "GNXEngineCommon.hlsl"
#include "StandardBRDF.hlsl"
#include "Lighting.hlsl"

//   ./dxc -T vs_6_7 -E VS ../../../../built-in/SkyBox_vert.hlsl -Fo ../../../../built-in/SkyBox_vert.spv -spirv
//   ./spirv-cross --version 300 --es ../../../built-in/SkyBox_vert.spv --output ../../../built-in/SkyBox_vert.glsl

// ./dxc -T ps_6_7 -E PS ../../../../built-in/SkyBox_frag.hlsl -Fo ../../../../built-in/SkyBox_frag.spv -spirv
// ./spirv-cross --version 300 --es ../../../built-in/SkyBox_frag.spv --output ../../../built-in/SkyBox_frag.glsl

// ./spirv-cross --msl --msl-ios  ../../../built-in/SkyBox_frag.spv --output ../../../built-in/SkyBox_frag.shader --rename-entry-point PS PS_Skybox frag

//#pragma multi_compile USE_GLES_30 USE_GLES_31

//天空盒的定义
struct SkyVertexIn
{
    float3 PosL    : POSITION;   //顶点坐标
};

struct SkyVertexOut
{
    float4 PosH : SV_POSITION;  //顶点输出坐标，裁剪空间
    float3 PosL : POSITION;
};

struct appdata_skybox
{
    float4 position : POSITION;
};

SkyVertexOut VS(appdata_skybox vin)
{
    SkyVertexOut vout;

    // Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.position.xyz;

    //MATRIX_V 只获取旋转的部分，去掉位移
    float4x4 viewMatrix = float4x4(MATRIX_V[0].x, MATRIX_V[0].y, MATRIX_V[0].z, 0.0,
									MATRIX_V[1].x, MATRIX_V[1].y, MATRIX_V[1].z, 0.0,
									MATRIX_V[2].x, MATRIX_V[2].y, MATRIX_V[2].z, 0.0,
									0.0, 0.0, 0.0, 1.0);
    //float4x4 viewMatrix = float4x4(float3x3(MATRIX_V));
    
    // Transform to world space.
    float4 posW = mul(vin.position, viewMatrix);
    posW = mul(posW, MATRIX_P);
    vout.PosH = posW.xyww;

    // Always center sky about camera.
//    posW.xyz += gEyePosW;
//
//    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    //vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

TextureCube gCubeMap : register(t0);

SamplerState gsamLinearWrap       : register(s0);

float4 PS(SkyVertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}



