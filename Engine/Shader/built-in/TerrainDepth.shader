//
//  TerrainDepth.shader
//  GNXEngine
//
//  Terrain depth pre-pass shader.
//  VS reads PatchMeta SSBO (via SV_InstanceID) and samples heightmap texture
//  to compute world-space vertex positions from the 17x17 template mesh.
//

#ifndef GNX_ENGINE_TERRAIN_DEPTH_HLSL
#define GNX_ENGINE_TERRAIN_DEPTH_HLSL

#include "GNXEngineVariables.hlsl"
#include "TerrainCommon.hlsl"

//=============================================================================
// Resources
//=============================================================================
StructuredBuffer<PatchMeta> gPatchMeta;
Texture2D<float> gHeightmap;
SamplerState gHeightmapSam;

//=============================================================================
// Vertex / Pixel shaders
//=============================================================================
struct VertexInput
{
    float3 position : POSITION;
    uint   instanceID : SV_InstanceID;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput VS(VertexInput input)
{
    PatchMeta meta = gPatchMeta[input.instanceID];

    // Compute world XZ from template local position + PatchMeta
    float worldX = meta.worldX + input.position.x * meta.worldSize;
    float worldZ = meta.worldZ + input.position.z * meta.worldSize;

    // Sample heightmap: convert world XZ to UV [0,1]
    float u = (worldX + _HalfWorldSize) / _WorldSize;
    float v = (worldZ + _HalfWorldSize) / _WorldSize;
    float height = gHeightmap.SampleLevel(gHeightmapSam, float2(u, v), 0);
    float worldY = height;  // heightmap already stores world-space Y

    // Transform to clip space (terrain is in world space, no model matrix needed)
    float4 worldPos = float4(worldX, worldY, worldZ, 1.0);
    float4 clipPos = mul(worldPos, MATRIX_VP);

    VertexOutput output;
    output.position = clipPos;
    return output;
}

void PS(VertexOutput input)
{
}

#endif // GNX_ENGINE_TERRAIN_DEPTH_HLSL
