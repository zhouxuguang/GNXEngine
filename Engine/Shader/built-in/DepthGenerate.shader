//
//  DepthGenerate.shader
//  GNXEngine
//
//  深度生成 Shader - 用于深度图像生成
//  将场景从光源视角渲染到深度纹理
//

#ifndef GNX_ENGINE_DEPTH_GENERATE_HLSL
#define GNX_ENGINE_DEPTH_GENERATE_HLSL

#include "GNXEngineVariables.hlsl"

//=============================================================================
// 输入输出结构
//=============================================================================

// 顶点着色器输入
struct DepthVertexInput
{
    float3 position : POSITION;
};

// 顶点着色器输出（几何着色器输入）
struct DepthVertexOutput
{
    float4 position : SV_POSITION;
};

//=============================================================================
// 顶点着色器
//=============================================================================
DepthVertexOutput VS(DepthVertexInput input)
{
    DepthVertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    // lightSpaceMatrix * model * position
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
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

#endif // GNX_ENGINE_DEPTH_GENERATE_HLSL
