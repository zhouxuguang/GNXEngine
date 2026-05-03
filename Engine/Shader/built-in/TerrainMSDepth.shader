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

struct TP { uint patchIndex; };
groupshared TP pl;

[numthreads(1, 1, 1)]
void TS(uint3 gid : SV_GroupID)
{
    uint idx = gid.x;
    if (idx >= gPatchCount)
        return;

    float4 ps[6];
    ExtractFrustumPlanes(MATRIX_VP, ps);

    PatchMeta m = gPatchMeta[idx];
    float3 mn = float3(m.worldX, m.minHeight, m.worldZ);
    float3 mx = float3(m.worldX + m.worldSize, m.minHeight + gMaxHeight, m.worldZ + m.worldSize);

    if (!AABBInFrustum(mn, mx, ps))
    {
        //return;
    }

    pl.patchIndex = idx;
    DispatchMesh(1, 1, 1, pl);
}

//=============================================================================
// Mesh Shader: 生成地形网格顶点 + 采样高度图
//=============================================================================

static const uint V = 9;          // 顶点网格边长
static const uint C = V - 1;      // 单元格边长
static const uint VC = V * V;     // 顶点总数
static const uint IC = C * C * 6; // 索引总数

struct VO { float4 pos : SV_Position; };

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void MS(out indices uint3 t[IC / 3], out vertices VO v[VC])
{
    SetMeshOutputCounts(VC, IC / 3);

    PatchMeta m = gPatchMeta[pl.patchIndex];

    // 生成顶点：在 patch 范围内采样高度图
    for (uint r = 0; r < V; r++)
    {
        for (uint c = 0; c < V; c++)
        {
            uint vi = r * V + c;
            float uf = (float)c / (V - 1);
            float vf = (float)r / (V - 1);

            float wx = m.worldX + uf * m.worldSize;
            float wz = m.worldZ + vf * m.worldSize;

            float tu = (wx + _HalfWorldSize) / _WorldSize;
            float tv = (wz + _HalfWorldSize) / _WorldSize;

            float h = gHeightmap.SampleLevel(gHeightmapSam, float2(tu, tv), 0);
            v[vi].pos = mul(float4(wx, h, wz, 1), MATRIX_VP);
        }
    }

    // 生成三角形索引
    uint ti = 0;
    for (uint r = 0; r < C; r++)
    {
        for (uint c = 0; c < C; c++)
        {
            uint v00 = r * V + c;
            uint v10 = v00 + 1;
            uint v01 = v00 + V;
            uint v11 = v01 + 1;

            t[ti++] = uint3(v00, v01, v11);
            t[ti++] = uint3(v00, v11, v10);
        }
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
