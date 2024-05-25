//
//  GNXEngineVariables.hlsl
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/31.
//

#ifndef GNX_ENGINE_VARIABLES_SGMDSFG
#define GNX_ENGINE_VARIABLES_SGMDSFG

//每一帧的结构
// cbuffer cbPerFrame : register(b0)
// {
    
// }

// 每一个相机的参数
cbuffer cbPerCamera : register(b0)
{
    float4x4 MATRIX_P;           //投影矩阵
    float4x4 MATRIX_INV_P;       //投影矩阵的逆矩阵
    float4x4 MATRIX_V;           //视图矩阵
    float4x4 MATRIX_INV_V;       //视图矩阵的逆矩阵
    float4x4 MATRIX_VP;          //视图投影矩阵
    float4x4 MATRIX_INV_VP;

    // 相机世界坐标位置
    float3 _WorldSpaceCameraPos;

    // x = 1 or -1 (-1 if projection is flipped)
    // y = near plane
    // z = far plane
    // w = 1/far plane
    float4 _ProjectionParams;

    // x = width
    // y = height
    // z = 1 + 1.0/width
    // w = 1 + 1.0/height
    float4 _ScreenParams;

    // Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
    // x = 1-far/near
    // y = far/near
    // z = x/far
    // w = y/far
    // or in case of a reversed depth buffer (UNITY_REVERSED_Z is 1)
    // x = -1+far/near
    // y = 1
    // z = x/far
    // w = 1/far
    float4 _ZBufferParams;

    // x = orthographic camera's width
    // y = orthographic camera's height
    // z = unused
    // w = 1.0 if camera is ortho, 0.0 if perspective
    //float4 _OrthoParams;
}

//每一个物体的结构
cbuffer cbPerObject : register(b1)
{
    float4x4 MATRIX_M;
    float4x4 MATRIX_M_INV;
    float4x4 MATRIX_Normal;
}

//灯光的信息
cbuffer cbLighting : register(b2)
{
    float4 _WorldSpaceLightPos;     //方向光: (world space direction, _WorldSpaceLightPos0.w = 0). Other lights: (world space position, _WorldSpaceLightPos0.w = 1).
    float3 _Strength;                // 光的强度
    float4 _LightColor;             // 光的颜色
    float _FalloffStart; // point/spot light only
    float _FalloffEnd;   // point/spot light only
    float _SpotPower;    // spot light only
}

cbuffer cbSkinned : register(b3)
{
    float4x4 pose[120];
    //float4x4 invBindPose[40];
};

#endif

