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
    float3 randomVec = normalize(texNoise.Sample(texNoiseSam, noiseUV).xyz);

    float3 biTang = cross(normal, randomVec);
    if (length(biTang) < 0.0001)
    {
        biTang = cross(normal, float3(0, 0, 1));
    }
    biTang = normalize(biTang);  // 必须归一化，确保TBN正交
    float3 tang = cross(biTang, normal);

    float3x3 TBN = float3x3(tang, biTang, normal);

    // vec3 randDir = normalize( texture(RandTex, TexCoord.xy *
    //         randScale).xyz );
    //         vec3 n = normalize( texture(NormalTex, TexCoord).xyz );
    //         vec3 biTang = cross( n, randDir );
    //         // If n and randDir are parallel, n is in x-y plane
    //         if( length(biTang) < 0.0001 )
    //           biTang = cross( n, vec3(0,0,1));
    //         biTang = normalize(biTang);
    //         vec3 tang = cross(biTang, n);
    //         mat3 toCamSpace = mat3(tang, biTang, n);
    
    // 构建TBN矩阵，用于将采样核心从切线空间变换到视图空间
    //float3x3 TBN = BuildTBNMatrix(normal);
    
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
        float3 sampleVec = viewPos + radius * mul(SampleKernel[i].xyz, TBN);
        
        // 将采样点投影到屏幕空间
        float4 offset = float4(sampleVec, 1.0f);
        offset = mul(offset, MATRIX_P);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5f + 0.5f;    // [-1,1] -> [0,1]
#ifdef TEXCOORD_FLIP
        offset.y = 1.0 - offset.y;
#endif
        
        // 获取采样点的深度
        float depthZ = gDepth.Sample(gDepthSam, offset.xy).r;
        float surfaceZ = ReconstructViewPosition(offset.xy, depthZ).z;
        float zDist = surfaceZ - viewPos.z;
        
        // 深度比较：如果采样点被遮挡，则增加遮蔽值
        // surfaceZ 和 sampleVec.z 都是视图空间Z值，与深度缓冲区布局(Reverse-Z/传统Z)无关
        // 视图空间: 相机沿-Z看，Z值越大(负得越少)越近
        // 如果实际表面比采样点更靠近相机(surfaceZ > sampleVec.z)，则采样点被遮挡
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(zDist));
        if (zDist >= 0.0 && zDist <= radius && surfaceZ > sampleVec.z) 
        {
            occlusion += 1.0;
        }
    }
    
    // 归一化遮蔽值
    occlusion = 1.0f - (occlusion / (float)kernelSize);
    
    // 增强对比度
    occlusion = pow(occlusion, 4.0f);
    
    return occlusion;
}
