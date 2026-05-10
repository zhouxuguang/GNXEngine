//
//  TerrainCull.shader
//  GNXEngine
//
//  Compute shader for terrain patch frustum culling.
//  Reads PatchMeta SSBO, tests each patch against the view frustum,
//  and outputs IndirectArgs (instanceCount = 1 for visible, 0 for culled).
//

#ifndef GNX_ENGINE_TERRAIN_CULL_HLSL
#define GNX_ENGINE_TERRAIN_CULL_HLSL

#include "GNXEngineVariables.hlsl"

//=============================================================================
// IndirectCommand struct (must match DrawIndexedIndirectCommand)
//=============================================================================
struct IndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

//=============================================================================
// Resources
//=============================================================================
StructuredBuffer<PatchMeta> gPatchMeta;
RWStructuredBuffer<IndirectCommand> gIndirectArgs;

cbuffer cbTerrainCull
{
    uint  gPatchCount;      // total number of patches
    float gMaxHeight;       // max height for AABB construction
    uint  gPad0;
    uint  gPad1;
};

//=============================================================================
// Compute Shader Entry Point
//=============================================================================
[numthreads(64, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gPatchCount) return;

    PatchMeta meta = gPatchMeta[idx];

    // Build AABB from PatchMeta
    float3 aabbMin = float3(meta.worldX, meta.minHeight, meta.worldZ);
    float3 aabbMax = float3(meta.worldX + meta.worldSize, meta.minHeight + gMaxHeight, meta.worldZ + meta.worldSize);

    bool visible = AABBInFrustum(aabbMin, aabbMax, frustumPlanes);

    // Write IndirectArgs
    IndirectCommand cmd;
    cmd.indexCount    = 384;               // 8x8 quads * 6 indices per quad (9x9 template mesh)
    cmd.instanceCount = visible ? 1 : 0;
    cmd.firstIndex    = 0;
    cmd.vertexOffset  = 0;
    cmd.firstInstance = 0;
    gIndirectArgs[idx] = cmd;
}

#endif // GNX_ENGINE_TERRAIN_CULL_HLSL
