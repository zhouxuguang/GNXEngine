#ifndef FEEDBACK_VIRTUAL_INCLUDE_GVHJH_H
#define FEEDBACK_VIRTUAL_INCLUDE_GVHJH_H

#include "VirtualTextureCommon.hlsl"
#include "GNXEngineVariables.hlsl"

//=============================================================================
// 输入输出结构
//=============================================================================

// 顶点着色器输入
struct DepthVertexInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

// 顶点着色器输出
struct DepthVertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

//=============================================================================
// 顶点着色器
//=============================================================================
DepthVertexOutput VS(DepthVertexInput input)
{
    DepthVertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);
    output.texCoord = input.texCoord;
    
    return output;
}

cbuffer cbVTFeedback
{
    float2 vtSize;
    float2 pageGrid;
    float2 minMaxMipLevel;
    float bufferScreenRatio;
    float pad;
}

//=============================================================================
// 像素着色器
//=============================================================================
uint PS(DepthVertexOutput input)
{
    float2 effectiveSize = vtSize * bufferScreenRatio;

    uint mipLevel = uint(clamp(
        ComputeMipLevel(input.texCoord.x, input.texCoord.y, effectiveSize.x, effectiveSize.y),
        minMaxMipLevel.x,
        minMaxMipLevel.y
    ));

    float mip_scale = exp2(-float(mipLevel));
    float2 curr_page_grid = max(pageGrid * mip_scale, float2(1.0, 1.0));

    float2 page_coords = floor(input.texCoord * curr_page_grid);
    page_coords.y = (curr_page_grid.y - 1) - page_coords.y;
    page_coords = clamp(page_coords, float2(0.0, 0.0), curr_page_grid - 1.0);

    uint data = PackPageData(mipLevel, uint(page_coords.x), uint(page_coords.y));

    return data;
}

#endif