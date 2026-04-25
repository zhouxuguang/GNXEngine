//
//  TerrainDepthInstanced.shader
//  GNXEngine
//
//  Depth-only shader for instanced terrain rendering.
//  Uses SSBO-based manual vertex fetch with SV_InstanceID.
//  Replaces per-leaf DrawIndexedPrimitives with single DrawIndexedInstancePrimitives.
//

#ifndef GNX_ENGINE_TERRAIN_DEPTH_INSTANCED_HLSL
#define GNX_ENGINE_TERRAIN_DEPTH_INSTANCED_HLSL

#include "GNXEngineVariables.hlsl"

//=============================================================================
// Terrain SSBOs — bound once at init, read-only
//=============================================================================

// Full terrain vertex buffer (global, shared by all patches)
// NOTE: Using float4 instead of float3 because MSL StructuredBuffer aligns float3 to 16 bytes.
StructuredBuffer<float4> _TerrainPositions   : register(t5);

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

struct DepthVertexOutput
{
    float4 position : SV_POSITION;
};

//=============================================================================
// Vertex Shader — manual vertex fetch via SSBOs + SV_InstanceID
//=============================================================================

DepthVertexOutput VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    DepthVertexOutput output;

    // Look up this patch's instance data
    PatchInstanceData patch = _PatchInstances[instanceID];

    // Fetch actual terrain index from master index buffer
    uint localIndex = _TerrainIndices[patch.indexStart + vertexID];

    // Apply baseVertex offset to get global vertex ID
    uint globalVertID = localIndex + patch.baseVertex;

    // Manual vertex fetch: only position needed for depth pass
    float3 position = _TerrainPositions[globalVertID].xyz;

    // Transform (same as DepthGenerate.VS)
    float4 worldPos = mul(float4(position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    return output;
}

//=============================================================================
// Pixel Shader — output depth value
//=============================================================================

float PS(DepthVertexOutput input) : SV_Depth
{
    return input.position.z;
}

#endif // GNX_ENGINE_TERRAIN_DEPTH_INSTANCED_HLSL
