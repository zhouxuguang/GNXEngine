#ifndef GNX_ENGINE_TERRAIN_BASEPASS_INSTANCED_HLSL
#define GNXENGINE_TERRAIN_BASEPASS_INSTANCED_HLSL

#include "GNXEngineVariables.hlsl"
#include "GBufferCommon.hlsl"

// 材质贴图 (same as BasePass.shader)
Texture2D gDiffuseMap;
SamplerState gDiffuseMapSam;
Texture2D gNormalMap;
SamplerState gNormalMapSam;
Texture2D gMetalRoughMap;
SamplerState gMetalRoughMapSam;
Texture2D gAmbientMap;
SamplerState gAmbientMapSam;
Texture2D gEmissiveMap;
SamplerState gEmissiveMapSam;

//=============================================================================
// Terrain SSBOs — bound once at init, read-only
//=============================================================================

// Full terrain vertex buffers (global, shared by all patches)
// NOTE: Using float4 instead of float3 for positions/normals because MSL StructuredBuffer
// aligns float3 to 16 bytes in a struct. Using float4 avoids stride mismatch.
StructuredBuffer<float4> _TerrainPositions   : register(t5);
StructuredBuffer<float4> _TerrainNormals     : register(t6);
StructuredBuffer<float4> _TerrainTangents    : register(t7);
StructuredBuffer<float2> _TerrainTexCoords   : register(t8);

// Master index buffer (static pool, all patches' indices concatenated)
StructuredBuffer<uint>   _TerrainIndices      : register(t9);

// Per-patch instance data (updated each frame after LOD selection + culling)
struct PatchInstanceData
{
    uint baseVertex;    // VB offset for this patch
    uint indexStart;    // Offset into _TerrainIndices for this patch's indices
    uint2 _pad;         // Padding to 16 bytes for StructuredBuffer alignment
};
StructuredBuffer<PatchInstanceData> _PatchInstances : register(t10);

//=============================================================================
// Input / Output
//=============================================================================

struct VertexOutput
{
    float4 position    : SV_POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    float4 prevClipPos : TEXCOORD1;
};

//=============================================================================
// Vertex Shader — manual vertex fetch via SSBOs + SV_InstanceID
//=============================================================================

VertexOutput VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VertexOutput output;

    // Look up this patch's instance data
    PatchInstanceData patch = _PatchInstances[instanceID];

    // Fetch actual terrain index from master index buffer
    // vertexID comes from dummy IB [0, 1, 2, ..., maxPatchIndexCount-1]
    uint localIndex = _TerrainIndices[patch.indexStart + vertexID];

    // Apply baseVertex offset to get global vertex ID
    uint globalVertID = localIndex + patch.baseVertex;

    // Manual vertex fetch from terrain SSBOs
    float3 position = _TerrainPositions[globalVertID].xyz;
    float3 normal   = _TerrainNormals[globalVertID].xyz;
    float4 tangent  = _TerrainTangents[globalVertID];
    float2 texCoord = _TerrainTexCoords[globalVertID];

    // Transform (same as BasePass.VS)
    float4 worldPos = mul(float4(position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    // Previous frame clip-space position (Motion Vector)
    float4 prevWorldPos = worldPos;
    output.prevClipPos = mul(prevWorldPos, MATRIX_PrevVP);

    float3 transformedNormal = mul(float4(normal.xyz, 0.0f), MATRIX_Normal).xyz;
    output.normal = transformedNormal;

    float3 transformedTangent = mul(float4(tangent.xyz, 0.0f), MATRIX_Normal).xyz;
    output.tangent.xyz = transformedTangent;
    output.tangent.w = tangent.w;

    output.texCoord = texCoord;

    return output;
}

//=============================================================================
// Fragment Output (same as BasePass.PS)
//=============================================================================

struct FragmentOutput
{
    float4 outRT0 : SV_TARGET0;   //scene color
    float4 outRT1 : SV_TARGET1;   //Normal + 0.33333f
    float4 outRT2 : SV_TARGET2;   //Metallic + Specular + Roughness + [4 bit 0b1010 | 4 bit ShadingModel]
    float4 outRT3 : SV_TARGET3;   //BaseColor + GenericAO
    float4 outRT4 : SV_TARGET4;   //Motion Vector (NDC空间偏移，RG通道)
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);
    float4 tangent = float4(normalize(input.tangent.xyz), input.tangent.w);

    // 采样法线贴图（如果有）
    float3 normalTS = gNormalMap.Sample(gNormalMapSam, input.texCoord).xyz;
    normalTS = normalTS * 2.0 - 1.0;  // [0,1] -> [-1,1]
    normal = NormalSampleToWorldSpace(normalTS, normal, tangent);

    // 采样材质贴图
    float4 baseColor = gDiffuseMap.Sample(gDiffuseMapSam, input.texCoord);
    float4 metalRough = gMetalRoughMap.Sample(gMetalRoughMapSam, input.texCoord);
    float ao = gAmbientMap.Sample(gAmbientMapSam, input.texCoord).r;
    float3 emissive = gEmissiveMap.Sample(gEmissiveMapSam, input.texCoord).rgb;

    FragmentOutput output;

    // RT0: Scene Color (写入自发光颜色，延迟渲染阶段会叠加光照)
    output.outRT0 = float4(emissive.rgb, 1.0f);

    // 法线编码
    normal = EncodeNormalOctahedron(normalize(normal));
    output.outRT1 = float4(normal, 0.333333f);

    // RT2: Metallic(0) + Specular(0.5) + Roughness(g通道) + [4 bit 0b1010 | 4 bit ShadingModel]
    uint lastCompoent = (10 << 4) | (1);
    output.outRT2 = float4(metalRough.r, 0.5f, metalRough.g, float(lastCompoent) / 255.0f);

    // RT3: BaseColor + AO
    output.outRT3 = float4(baseColor.rgb, ao);

    // RT4: Motion Vector（NDC 空间偏移）
    float2 motionVector = float2(0.0, 0.0);
    if (input.prevClipPos.w != 0.0)
    {
        float2 curNDC = input.position.xy / input.position.w;
        float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
        motionVector = curNDC - prevNDC;
    }
    output.outRT4 = float4(motionVector, 0.0, 0.0);

    return output;
}

#endif
