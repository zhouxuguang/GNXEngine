//
//  SSAOBlur.hlsl
//  GNXEngine
//
//  SSAO模糊Shader - 支持Box模糊、高斯模糊、双边模糊
//

#include "GNXEngineCommon.hlsl"

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
#ifdef TEXCOORD_FLIP
    vout.texCoord.y = 1.0 - vout.texCoord.y;
#endif
    
    return vout;
}

// SSAO纹理
Texture2D texSSAO;
SamplerState texSSAOSam;

// 深度纹理（用于双边模糊）
Texture2D gDepth;
SamplerState gDepthSam;

// 模糊参数
// 使用引擎内置的 _ScreenParams: x = width, y = height
#define BLUR_RADIUS 2
#define USE_BILATERAL_BLUR 1

// Box模糊（简单平均）
float BoxBlur(float2 uv, int radius)
{
    float result = 0.0f;
    float count = 0.0f;
    
    float2 texelSize = 1.0f / _ScreenParams.xy;
    
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            float2 offsetUV = uv + float2(x, y) * texelSize;
            result += texSSAO.Sample(texSSAOSam, offsetUV).r;
            count += 1.0f;
        }
    }
    
    return result / count;
}

// 高斯模糊权重
static const float GaussianWeights[3] = { 0.227027f, 0.316216f, 0.070270f };

// 高斯模糊（可分离的两Pass版本，这里使用单Pass近似）
float GaussianBlur(float2 uv, int radius)
{
    float result = 0.0f;
    float totalWeight = 0.0f;
    
    float2 texelSize = 1.0f / _ScreenParams.xy;
    
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            float2 offsetUV = uv + float2(x, y) * texelSize;
            
            // 计算高斯权重（简化版）
            float dist = length(float2(x, y));
            float weight = exp(-dist * dist / 2.0f);
            
            result += texSSAO.Sample(texSSAOSam, offsetUV).r * weight;
            totalWeight += weight;
        }
    }
    
    return result / totalWeight;
}

// 双边模糊 - 在模糊边缘的同时保留深度不连续
float BilateralBlur(float2 uv, int radius)
{
    float result = 0.0f;
    float totalWeight = 0.0f;
    
    float2 texelSize = 1.0f / _ScreenParams.xy;
    
    // 中心深度
    float centerDepth = gDepth.Sample(gDepthSam, uv).r;
    
    // 中心SSAO值
    float centerSSAO = texSSAO.Sample(texSSAOSam, uv).r;
    
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            float2 offsetUV = uv + float2(x, y) * texelSize;
            
            // 采样深度
            float sampleDepth = gDepth.Sample(gDepthSam, offsetUV).r;
            
            // 采样SSAO
            float sampleSSAO = texSSAO.Sample(texSSAOSam, offsetUV).r;
            
            // 空间权重（高斯）
            float dist = length(float2(x, y));
            float spatialWeight = exp(-dist * dist / 2.0f);
            
            // 深度权重（防止边缘模糊）
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-depthDiff * depthDiff * 1000.0f);
            
            // 组合权重
            float weight = spatialWeight * depthWeight;
            
            result += sampleSSAO * weight;
            totalWeight += weight;
        }
    }
    
    return result / totalWeight;
}

// 像素着色器 - SSAO模糊
float PS(VertexOut pin) : SV_Target0
{
#if USE_BILATERAL_BLUR
    // 双边模糊 - 保留边缘
    return BilateralBlur(pin.texCoord, BLUR_RADIUS);
#elif 0
    // 高斯模糊
    return GaussianBlur(pin.texCoord, BLUR_RADIUS);
#else
    // Box模糊
    return BoxBlur(pin.texCoord, BLUR_RADIUS);
#endif
}
