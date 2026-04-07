//
//  MotionBlur.hlsl
//  GNXEngine
//
//  运动模糊后处理Shader
//

#include "GNXEngineCommon.hlsl"
#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// 全屏四边形顶点着色器
VertexOut VS(uint vertexID : SV_VertexID)
{
    VertexOut vout;
    
    vout.PosH = fsTrianglePosition(vertexID);  
    vout.texCoord = fsTriangleUV(vertexID);  
    
    return vout;
}

// 场景颜色纹理
Texture2D gColorTexture;
SamplerState gColorTextureSam;

// 深度纹理
Texture2D gDepthTexture;
SamplerState gDepthTextureSam;

// Motion Blur参数
#define INTENSITY 1.0f
#define NUM_SAMPLES 16
#define MAX_VELOCITY 0.05f  // 最大速度限制（屏幕空间的百分比）

// 像素着色器 - Motion Blur计算
float4 PS(VertexOut pin) : SV_Target0
{
    // 获取深度值
    float depth = gDepthTexture.Sample(gDepthTextureSam, pin.texCoord).r;
    
    // 重建世界空间位置
    // H是视口位置，范围[-1,1]
    float4 H = float4(pin.texCoord.x * 2.0 - 1.0, (1.0 - pin.texCoord.y) * 2.0 - 1.0, depth, 1.0);
    
#ifdef TEXCOORD_FLIP
    H.y = (pin.texCoord.y * 2.0 - 1.0);
#endif
    
    // 使用当前帧的逆VP矩阵变换到世界空间
    float4 D = mul(H, MATRIX_INV_VP);
    float4 worldPos = D / D.w;
    
    // 使用上一帧的VP矩阵变换到上一帧的屏幕空间
    float4 previousPos = mul(worldPos, MATRIX_PrevVP);
    previousPos /= previousPos.w;
    
    // 计算速度向量（屏幕空间位移）
    // 当前帧的NDC位置
    float2 currentPos = H.xy;
    // 上一帧的NDC位置
    float2 prevScreenPos = previousPos.xy;
    
    // 速度向量 = 当前位置 - 上一帧位置
    float2 velocity = (currentPos - prevScreenPos) * INTENSITY * 0.5;
    
    // 如果速度太小，直接返回原色
    float speed = length(velocity);
    if (speed < 0.0001)
    {
        return gColorTexture.Sample(gColorTextureSam, pin.texCoord);
    }
    
    // 沿速度方向采样
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    
    // 限制最大采样范围，防止过度模糊
    velocity = normalize(velocity) * min(speed, MAX_VELOCITY);
    
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        // 沿速度方向采样，中心权重更高
        float t = (float(i) / float(NUM_SAMPLES - 1)) - 0.5;
        float2 sampleUV = pin.texCoord + velocity * t * 2.0;
        
        // 边界检查
        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 &&
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0)
        {
            color += gColorTexture.Sample(gColorTextureSam, sampleUV);
        }
        else
        {
            // 边界外使用原色
            color += gColorTexture.Sample(gColorTextureSam, pin.texCoord);
        }
    }
    
    // 平均
    color /= float(NUM_SAMPLES);
    
    return color;
}
