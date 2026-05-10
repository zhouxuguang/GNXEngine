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
        // Frustum culling
        PatchMeta m = gPatchMeta[dtid];
        float3 mn = float3(m.worldX, m.minHeight, m.worldZ);
        float3 mx = float3(m.worldX + m.worldSize, m.minHeight + gMaxHeight, m.worldZ + m.worldSize);
        visible = AABBInFrustum(mn, mx, frustumPlanes);
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
        uint row = vi / V;
        uint col = vi % V;
        float uf = (float)col / (V - 1);
        float vf = (float)row / (V - 1);

        float wx = m.worldX + uf * m.worldSize;
        float wz = m.worldZ + vf * m.worldSize;

        float tu = (wx + _HalfWorldSize) / _WorldSize;
        float tv = (wz + _HalfWorldSize) / _WorldSize;

        float h = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, tv), 0);

        // 裂缝处理，取更粗patch边上上相邻两点的平均值作为新的高程
        uint nf = m.neighborFlags;
        if (nf != 0)
        {
            if (col == 0 && (row & 1) && (nf & 1u))          // left(-X) coarser
            {
                float wz0 = m.worldZ + (float)(row - 1) / (float)(V - 1) * m.worldSize;
                float wz1 = m.worldZ + (float)(row + 1) / (float)(V - 1) * m.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, (wz0 + _HalfWorldSize) / _WorldSize), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, (wz1 + _HalfWorldSize) / _WorldSize), 0);
                h = (h0 + h1) * 0.5;
            }
            else if (col == V - 1 && (row & 1) && (nf & 2u)) // right(+X) coarser
            {
                float wz0 = m.worldZ + (float)(row - 1) / (float)(V - 1) * m.worldSize;
                float wz1 = m.worldZ + (float)(row + 1) / (float)(V - 1) * m.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, (wz0 + _HalfWorldSize) / _WorldSize), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, (wz1 + _HalfWorldSize) / _WorldSize), 0);
                h = (h0 + h1) * 0.5;
            }
            else if (row == V - 1 && (col & 1) && (nf & 4u)) // bottom(+Z) coarser
            {
                float wx0 = m.worldX + (float)(col - 1) / (float)(V - 1) * m.worldSize;
                float wx1 = m.worldX + (float)(col + 1) / (float)(V - 1) * m.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx0 + _HalfWorldSize) / _WorldSize, tv), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx1 + _HalfWorldSize) / _WorldSize, tv), 0);
                h = (h0 + h1) * 0.5;
            }
            else if (row == 0 && (col & 1) && (nf & 8u))      // top(-Z) coarser
            {
                float wx0 = m.worldX + (float)(col - 1) / (float)(V - 1) * m.worldSize;
                float wx1 = m.worldX + (float)(col + 1) / (float)(V - 1) * m.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx0 + _HalfWorldSize) / _WorldSize, tv), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx1 + _HalfWorldSize) / _WorldSize, tv), 0);
                h = (h0 + h1) * 0.5;
            }
        }

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
// Pixel Shader: 深度由光栅化器自动写入，无需手动输出
//=============================================================================

void PS(VO input)
{
}

#endif
