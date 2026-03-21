//
//  HiZGeneration.shader
//  GNXEngine
//
//  Hi-Z (Hierarchical Z-Buffer) 生成Compute Shader
//
//  Created by zhouxuguang
//

#include "GNXEngineCommon.hlsl"

//=============================================================================
// 常量缓冲区
//=============================================================================

cbuffer HiZParams
{
    uint2 textureSize;          // 当前层级的纹理尺寸
    uint mipLevel;              // 当前生成的Mip Level
    uint padding;
    
    float2 invTextureSize;      // 1.0 / textureSize
    float2 padding2;
};

// 输入：上一层的深度纹理（Level 0为原始深度缓冲）
Texture2D srcDepth;
SamplerState srcSampler;

// 输出：当前层的Hi-Z纹理（作为UAV）
RWTexture2D<float> dstDepth;

// 线程组大小：8x8，每个线程处理2x2像素块
[numthreads(8, 8, 1)]
void CS(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchThreadId : SV_DispatchThreadID,
    uint groupIndex : SV_GroupIndex)
{
    // 检查是否在纹理范围内
    if (dispatchThreadId.x >= textureSize.x || dispatchThreadId.y >= textureSize.y)
    {
        return;
    }
    
    // 计算2x2邻域的采样位置（在上一层纹理空间）
    // 每个线程处理一个2x2的像素块
    int2 basePos = int2(dispatchThreadId.xy) * 2;
    
    // 采样四个深度值
    // 使用texelFetch确保精确的像素采样，不进行过滤
    float depth00 = srcDepth[basePos + int2(0, 0)].r;
    float depth01 = srcDepth[basePos + int2(0, 1)].r;
    float depth10 = srcDepth[basePos + int2(1, 0)].r;
    float depth11 = srcDepth[basePos + int2(1, 1)].r;
    
    // 根据深度模式选择取最大值还是最小值
    float result;
    
#ifdef USE_REVERSE_Z
    // Reverse-Z模式：
    // - 近处深度值 = 1.0
    // - 远处深度值 = 0.0
    // - 取最小值（保守估计，选择最远的可见深度）
    result = min(min(min(depth00, depth01), depth10), depth11);
#else
    // 传统Z模式：
    // - 近处深度值 = 0.0
    // - 远处深度值 = 1.0
    // - 取最大值（保守估计，选择最远的可见深度）
    result = max(max(max(depth00, depth01), depth10), depth11);
#endif
    
    // 写入当前层的Hi-Z纹理
    dstDepth[dispatchThreadId.xy] = result;
}

//=============================================================================
// 调试可视化（可选）
//=============================================================================

#ifdef DEBUG_HIZ_VISUALIZATION

// 用于调试：可视化Hi-Z纹理
Texture2D hizTexture : register(t0);
SamplerState hizSampler : register(s1);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 PSMain(VSOutput input) : SV_Target
{
    // 采样Hi-Z纹理
    float depth = hizTexture.Sample(hizSampler, input.texcoord).r;
    
    // 线性化深度用于可视化
    float linearDepth = depth; // 实际应该根据投影矩阵转换
    
    // 转换为灰度颜色
    return float4(linearDepth.xxx, 1.0);
}

#endif // DEBUG_HIZ_VISUALIZATION
