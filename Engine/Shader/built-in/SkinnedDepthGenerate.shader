//
//  DepthGenerate.shader
//  GNXEngine
//
//  深度生成 Shader - 用于深度图像生成
//  将场景从光源视角渲染到深度纹理
//

#ifndef GNX_ENGINE_SKINNED_DEPTH_GENERATE_HLSL
#define GNX_ENGINE_SKINNED_DEPTH_GENERATE_HLSL

#include "GNXEngineVariables.hlsl"

//=============================================================================
// 输入输出结构
//=============================================================================

// 顶点着色器输入
struct DepthVertexInput
{
    float3 position : POSITION;
    uint4 boneIndex : BONEINDICES;
    float4 weight : WEIGHTS;
};

// 顶点着色器输出（几何着色器输入）
struct DepthVertexOutput
{
    float4 position : SV_POSITION;
};

//=============================================================================
// 顶点着色器
//=============================================================================
DepthVertexOutput VS(DepthVertexInput vin)
{
    DepthVertexOutput output;

    float4x4 skin = (pose[vin.boneIndex.x]) * vin.weight.x;
    skin += (pose[vin.boneIndex.y]) * vin.weight.y;
    skin += (pose[vin.boneIndex.z]) * vin.weight.z;
    skin += (pose[vin.boneIndex.w]) * vin.weight.w;

    float4x4 modelMatrix = mul(skin, MATRIX_M);
    
    // 将顶点位置从模型空间变换到裁剪空间
    float4 worldPos = mul(float4(vin.position.xyz, 1.0), modelMatrix);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);
    
    return output;
}

//=============================================================================
// 像素着色器
//=============================================================================
float PS(DepthVertexOutput input) : SV_Depth
{
    // 使用默认深度值（SV_Position 的 z 分量会自动写入深度缓冲）
    // 如果需要自定义深度，可以返回 float 值
    return input.position.z;
}

#endif // GNX_ENGINE_SKINNED_DEPTH_GENERATE_HLSL
