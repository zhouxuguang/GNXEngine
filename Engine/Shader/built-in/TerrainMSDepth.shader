//
// TerrainMSDepth.shader
// GNXEngine - Terrain Depth-only via Task + Mesh Shader.
//

#ifndef GNX_ENGINE_TERRAIN_MS_DEPTH_HLSL
#define GNX_ENGINE_TERRAIN_MS_DEPTH_HLSL

#include "GNXEngineVariables.hlsl"
#include "TerrainCommon.hlsl"

StructuredBuffer<PatchMeta> gPatchMeta;
Texture2D<float> gHeightmap;
SamplerState gHeightmapSam;

//=============================================================================
// Task Shader: 视锥体剔除
//=============================================================================

groupshared TerrainPayload terrainPayload;

[numthreads(32, 1, 1)]
void TS(uint gid : SV_GroupID, uint gtid : SV_GroupIndex, uint dtid : SV_DispatchThreadID)
{
    // Initialize payload to sentinel — prevents stale groupshared data from causing ghost patches
    terrainPayload.patchIndices[gtid] = 0xFFFFFFFF;

    bool visible = false;

    if (dtid < gPatchCount)
    {
        visible = true;  // Culling disabled — all valid patches are visible

        // Frustum culling (disabled for debugging)
        float4 ps[6];
        ExtractFrustumPlanes(MATRIX_VP, ps);
        PatchMeta m = gPatchMeta[dtid];
        float3 mn = float3(m.worldX, m.minHeight, m.worldZ);
        float3 mx = float3(m.worldX + m.worldSize, m.minHeight + gMaxHeight, m.worldZ + m.worldSize);
        //visible = AABBInFrustum(mn, mx, ps);
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        terrainPayload.patchIndices[index] = dtid;
    }

    uint visibleCount = WaveActiveCountBits(visible);

    DispatchMesh(visibleCount, 1, 1, terrainPayload);
}

//=============================================================================
// Mesh Shader: 生成地形网格顶点 + 采样高度图 (32 threads)
//=============================================================================

static const uint V  = 9;            // vertices per side
static const uint C  = V - 1;        // cells per side
static const uint VC = V * V;        // total vertices = 81
static const uint TC = C * C * 2;    // total triangles = 128

struct VO { float4 pos : SV_Position; };

[outputtopology("triangle")]
[numthreads(32, 1, 1)]
void MS(out indices uint3 triangles[TC], out vertices VO v[VC],
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID)
{
    uint patchIndex = terrainPayload.patchIndices[gid];

    if (patchIndex >= gPatchCount)
    {
        return;
    }

    PatchMeta m = gPatchMeta[patchIndex];

    // All threads must call SetMeshOutputCounts with the same values
    SetMeshOutputCounts(VC, TC);

    // ---- Vertex generation: each thread handles ceil(81/32)=3 vertices ----
    for (uint vi = gtid; vi < VC; vi += 32)
    {
        float uf = (float)(vi % V) / (V - 1);
        float vf = (float)(vi / V) / (V - 1);

        float wx = m.worldX + uf * m.worldSize;
        float wz = m.worldZ + vf * m.worldSize;

        float tu = (wx + _HalfWorldSize) / _WorldSize;
        float tv = (wz + _HalfWorldSize) / _WorldSize;

        float h = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, tv), 0);
        v[vi].pos = mul(float4(wx, h, wz, 1), MATRIX_VP);
    }

    // ---- Index generation: each thread handles ceil(64/32)=2 cells ----
    for (uint ci = gtid; ci < C * C; ci += 32)
    {
        uint r = ci / C;
        uint c = ci % C;
        uint v00 = r * V + c;
        uint v10 = v00 + 1;
        uint v01 = v00 + V;
        uint v11 = v01 + 1;

        triangles[ci * 2 + 0] = uint3(v00, v01, v11);
        triangles[ci * 2 + 1] = uint3(v00, v11, v10);
    }
}

//=============================================================================
// Pixel Shader: 仅输出深度
//=============================================================================

float PS(VO input) : SV_Depth
{
    return input.pos.z;
}

#endif
