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
// PatchMeta struct (must match C++ definition)
//=============================================================================
struct PatchMeta
{
    float worldX;
    float worldZ;
    float worldSize;
    float minHeight;
    uint  gridX;
    uint  gridZ;
    uint  gridSize;
    uint  level;
};

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
// Frustum plane extraction from VP matrix
//=============================================================================
void ExtractFrustumPlanes(float4x4 vp, out float4 planes[6])
{
    // Row-vector convention: plane = vp[row3] +/- vp[rowN]
    // Left
    planes[0] = vp[3] + vp[0];
    // Right
    planes[1] = vp[3] - vp[0];
    // Bottom
    planes[2] = vp[3] + vp[1];
    // Top
    planes[3] = vp[3] - vp[1];
    // Near
    planes[4] = vp[3] + vp[2];
    // Far
    planes[5] = vp[3] - vp[2];

    // Normalize planes
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float len = length(planes[i].xyz);
        planes[i] /= len;
    }
}

//=============================================================================
// AABB vs Frustum test
//=============================================================================
bool AABBInFrustum(float3 aabbMin, float3 aabbMax, float4 planes[6])
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // Find the "most positive" corner along the plane normal
        float3 p = float3(
            planes[i].x > 0 ? aabbMin.x : aabbMax.x,
            planes[i].y > 0 ? aabbMin.y : aabbMax.y,
            planes[i].z > 0 ? aabbMin.z : aabbMax.z
        );
        if (dot(planes[i].xyz, p) + planes[i].w < 0)
            return false;
    }
    return true;
}

//=============================================================================
// Compute Shader Entry Point
//=============================================================================
[numthreads(64, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gPatchCount) return;

    // Extract frustum from VP matrix
    float4 planes[6];
    ExtractFrustumPlanes(MATRIX_VP, planes);

    PatchMeta meta = gPatchMeta[idx];

    // Build AABB from PatchMeta
    float3 aabbMin = float3(meta.worldX, meta.minHeight, meta.worldZ);
    float3 aabbMax = float3(meta.worldX + meta.worldSize, meta.minHeight + gMaxHeight, meta.worldZ + meta.worldSize);

    bool visible = AABBInFrustum(aabbMin, aabbMax, planes);

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
