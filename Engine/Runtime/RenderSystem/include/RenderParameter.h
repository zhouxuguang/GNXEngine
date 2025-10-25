#include "RSDefine.h"
#include "RenderCore/VertexBuffer.h"
#include "RenderCore/RenderDevice.h"
#include "MathUtil/SimdMath.h"

NS_RENDERSYSTEM_BEGIN

struct cbPerCamera
{
    mathutil::Matrix4x4f MATRIX_P;           //投影矩阵
    mathutil::Matrix4x4f MATRIX_INV_P;       //投影矩阵的逆矩阵
    mathutil::Matrix4x4f MATRIX_V;           //视图矩阵
    mathutil::Matrix4x4f MATRIX_INV_V;       //视图矩阵的逆矩阵
    mathutil::Matrix4x4f MATRIX_VP;          //视图投影矩阵
    mathutil::Matrix4x4f MATRIX_INV_VP;

    // 相机世界坐标位置
    mathutil::simd_float3 _WorldSpaceCameraPos;

    // x = 1 or -1 (-1 if projection is flipped)
    // y = near plane
    // z = far plane
    // w = 1/far plane
    mathutil::simd_float4 _ProjectionParams;

    // x = width
    // y = height
    // z = 1 + 1.0/width
    // w = 1 + 1.0/height
    mathutil::simd_float4 _ScreenParams;

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
    mathutil::simd_float4 _ZBufferParams;
};

struct cbLighting
{
    mathutil::simd_float4 WorldSpaceLightPos;     //方向光: (world space direction, _WorldSpaceLightPos0.w = 0). Other lights: (world space position, _WorldSpaceLightPos0.w = 1).
    mathutil::simd_float3 Strength;                // 光的强度
    mathutil::simd_float4 LightColor;             // 光的颜色
    float FalloffStart; // point/spot light only
    float FalloffEnd;   // point/spot light only
    float SpotPower;    // spot light only
};

//每一个物体的结构
struct cbPerObject
{
    mathutil::Matrix4x4f MATRIX_M;
    mathutil::Matrix4x4f MATRIX_M_INV;
    mathutil::Matrix4x4f MATRIX_Normal;
};

NS_RENDERSYSTEM_END
