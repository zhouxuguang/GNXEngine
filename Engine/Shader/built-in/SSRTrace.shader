// Screen-space reflection ray tracing pass.

#include "GNXEngineCommon.hlsl"
#include "GBufferCommon.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut VS(uint vertexID : SV_VertexID)
{
    VertexOut output;
    output.PosH = fsTrianglePosition(vertexID);
    output.texCoord = fsTriangleUV(vertexID);
    return output;
}

Texture2D gSceneColor;
SamplerState gSceneColorSam;
Texture2D gGBufferA;
SamplerState gGBufferASam;
Texture2D gGBufferB;
SamplerState gGBufferBSam;
Texture2D gDepth;
SamplerState gDepthSam;

cbuffer cbSSRParams
{
    float4 TraceParams; // max distance, step length, thickness, max steps
    float4 FadeParams;  // edge fade, max roughness, intensity, binary steps
}

float2 ProjectViewPosition(float3 viewPosition)
{
    float4 clip = mul(float4(viewPosition, 1.0), MATRIX_P);
    float2 uv = clip.xy / clip.w * 0.5 + 0.5;
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    return uv;
}

bool IsOutside(float2 uv)
{
    return any(uv <= 0.0) || any(uv >= 1.0);
}

float4 PS(VertexOut input) : SV_Target0
{
    float2 uv = input.texCoord;
    float depth = gDepth.Sample(gDepthSam, uv).r;
#ifdef USE_REVERSE_Z
    if (depth <= 0.00001)
        return 0.0;
#else
    if (depth >= 0.99999)
        return 0.0;
#endif

    float4 packedNormal = gGBufferA.Sample(gGBufferASam, uv);
    // BasePass stores perceptual roughness in GBufferB.b.
    float roughness = gGBufferB.Sample(gGBufferBSam, uv).b;
    if (roughness >= FadeParams.y)
        return 0.0;

    float3 viewPosition = ReconstructViewPosition(uv, depth);
    float3 worldNormal = DecodeNormalOctahedron(packedNormal.xyz);
    float3 viewNormal = normalize(mul(worldNormal, (float3x3)MATRIX_V));
    float3 incident = normalize(viewPosition);
    float3 rayDirection = normalize(reflect(incident, viewNormal));

    // A ray travelling towards or parallel to the camera cannot hit visible geometry.
    if (rayDirection.z >= -0.001)
        return 0.0;

    float maxDistance = TraceParams.x;
    float stepLength = max(TraceParams.y, 0.01);
    float thickness = max(TraceParams.z, 0.001);
    int maxSteps = min((int)TraceParams.w, 256);
    int binarySteps = min((int)FadeParams.w, 8);

    float3 previousPosition = viewPosition + viewNormal * 0.03;
    float3 rayPosition = previousPosition;
    float travelled = 0.0;
    float2 hitUV = 0.0;
    bool hit = false;

    [loop]
    for (int i = 0; i < maxSteps; ++i)
    {
        previousPosition = rayPosition;
        rayPosition += rayDirection * stepLength;
        travelled += stepLength;
        if (travelled > maxDistance)
            break;

        float2 sampleUV = ProjectViewPosition(rayPosition);
        if (IsOutside(sampleUV))
            break;

        float sampleDepth = gDepth.SampleLevel(gDepthSam, sampleUV, 0).r;
#ifdef USE_REVERSE_Z
        if (sampleDepth <= 0.00001)
            continue;
#else
        if (sampleDepth >= 0.99999)
            continue;
#endif
        float sceneZ = ReconstructViewPosition(sampleUV, sampleDepth).z;
        float depthDelta = sceneZ - rayPosition.z;
        if (depthDelta >= 0.0 && depthDelta <= thickness)
        {
            float3 front = previousPosition;
            float3 back = rayPosition;
            [unroll]
            for (int j = 0; j < binarySteps; ++j)
            {
                float3 middle = (front + back) * 0.5;
                float2 middleUV = ProjectViewPosition(middle);
                float middleDepth = gDepth.SampleLevel(gDepthSam, middleUV, 0).r;
                float middleSceneZ = ReconstructViewPosition(middleUV, middleDepth).z;
                if (middleSceneZ - middle.z >= 0.0)
                    back = middle;
                else
                    front = middle;
            }
            hitUV = ProjectViewPosition((front + back) * 0.5);
            hit = !IsOutside(hitUV);
            break;
        }
    }

    if (!hit)
        return 0.0;

    float2 edgeDistance = min(hitUV, 1.0 - hitUV);
    float edgeFade = saturate(min(edgeDistance.x, edgeDistance.y) / max(FadeParams.x, 0.001));
    float distanceFade = 1.0 - saturate(travelled / maxDistance);
    float angleFade = saturate(-rayDirection.z * 4.0);
    float roughnessFade = 1.0 - saturate(roughness / FadeParams.y);
    float confidence = edgeFade * distanceFade * angleFade * roughnessFade;
    float3 reflection = gSceneColor.SampleLevel(gSceneColorSam, hitUV, 0).rgb;
    return float4(reflection, confidence);
}
