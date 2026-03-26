//
//  SSAO.hlsl
//  GNXEngine
//
//  屏幕空间环境光遮蔽(SSAO) Shader
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
#ifdef TEXCOORD_FLIP
    vout.texCoord.y = 1.0 - vout.texCoord.y;
#endif
    
    return vout;
}

// G-Buffer纹理
Texture2D gGBufferA;    // Normal (xyz)
SamplerState gGBufferASam;

Texture2D gDepth;       // 深度纹理
SamplerState gDepthSam;

// 噪声纹理
Texture2D texNoise;
SamplerState texNoiseSam;

// SSAO参数
cbuffer cbSSAOParams
{
    float4 SampleKernel[64];         // 采样核心
    float4 noiseScale;          // 噪声缩放 (width/noiseSize, height/noiseSize, 0, 0)
    float radius;               // 采样半径
    float bias;                 // 深度偏移
    int kernelSize;             // 采样核心大小
    float padding;
}

// 构建TBN矩阵（从法线）
float3x3 BuildTBNMatrix(float3 normal)
{
    // 选择一个与法线不平行的向量
    float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    
    // 计算切线和副切线
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    
    // 构建TBN矩阵
    return float3x3(tangent, bitangent, normal);
}

// 像素着色器 - SSAO计算
float PS(VertexOut pin) : SV_Target0
{
    // 从G-Buffer获取法线（octahedral编码，世界空间）
    float3 worldNormal = DecodeNormalOctahedron(gGBufferA.Sample(gGBufferASam, pin.texCoord).xyz);
    
    // 将世界空间法线转换到视图空间（SSAO在视图空间计算）
    float3 normal = normalize(mul(worldNormal, (float3x3)MATRIX_V));
    
    // 从深度重建视图空间位置
    float depth = gDepth.Sample(gDepthSam, pin.texCoord).r;
    float3 viewPos = ReconstructViewPosition(pin.texCoord, depth);
    
    // 获取随机旋转向量
    float2 noiseUV = pin.texCoord * noiseScale.xy;
    float3 randomVec = texNoise.Sample(texNoiseSam, noiseUV).xyz;
    
    // 构建TBN矩阵，用于将采样核心从切线空间变换到视图空间
    float3x3 TBN = BuildTBNMatrix(normal);
    
    // SSAO遮挡计算
    float occlusion = 0.0f;
    
    for (int i = 0; i < kernelSize; ++i)
    {
        // vec3 samplePos = camPos + Radius * (toCamSpace * SampleKernel[i]);

        // // Project point
        // vec4 p = ProjectionMatrix * vec4(samplePos,1);
        // p *= 1.0 / p.w;
        // p.xyz = p.xyz * 0.5 + 0.5;

        // // Access camera space z-coordinate at that point
        // float surfaceZ = texture(PositionTex, p.xy).z;
        // float zDist = surfaceZ - camPos.z;
        
        // // Count points that ARE occluded
        // if( zDist >= 0.0 && zDist <= Radius && surfaceZ > samplePos.z ) occlusionSum += 1.0;

        // 将采样点从切线空间变换到视图空间
        float3 sampleVec = mul(SampleKernel[i].xyz, TBN);
        
        // 计算采样点在视图空间的位置
        float3 samplePos = viewPos + sampleVec * radius;
        
        // 将采样点投影到屏幕空间
        float4 offset = float4(samplePos, 1.0f);
        offset = mul(offset, MATRIX_P);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5f + 0.5f;    // [-1,1] -> [0,1]
        
        // 获取采样点的深度
        float sampleDepth = gDepth.Sample(gDepthSam, offset.xy).r;
        
        // 重建采样点的视图空间位置
        float3 sampleViewPos = ReconstructViewPosition(offset.xy, sampleDepth);
        
        // Range check：确保采样点在合理的范围内
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(viewPos.z - sampleViewPos.z));
        
        // 深度比较：如果采样点被遮挡，则增加遮蔽值
#ifdef REVERSE_Z
        // Reverse-Z: 深度值越大越近，越小越远
        occlusion += (sampleDepth <= offset.z - bias ? 1.0f : 0.0f) * rangeCheck;
#else
        // 传统Z: 深度值越小越近，越大越远
        occlusion += (sampleDepth >= offset.z + bias ? 1.0f : 0.0f) * rangeCheck;
#endif
    }
    
    // 归一化遮蔽值
    occlusion = 1.0f - (occlusion / (float)kernelSize);
    
    // 增强对比度
    occlusion = pow(occlusion, 4.0f);
    
    return occlusion;
}
