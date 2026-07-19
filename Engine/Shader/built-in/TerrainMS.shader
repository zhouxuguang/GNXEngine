//
// TerrainMS.shader
// GNXEngine - Terrain via Task + Mesh Shader pipeline.
// TS: Per-patch frustum culling -> DispatchMesh
// MS: 9x9 grid vertex expansion + heightmap sampling
// PS: G-Buffer output (5 RTs)
//

#ifndef GNX_ENGINE_TERRAIN_MS_HLSL
#define GNX_ENGINE_TERRAIN_MS_HLSL

#include "GNXEngineVariables.hlsl"
#include "GBufferCommon.hlsl"
#include "TerrainCommon.hlsl"

//=============================================================================
// Resources
//=============================================================================

StructuredBuffer<PatchMeta> gPatchMeta;    // Task stage: all leaf patches
Texture2D<float> gHeightmap;               // Mesh stage: heightmap
SamplerState gHeightmapSam;

Texture2D gDiffuseMap;                     // Fragment stage: diffuse
SamplerState gDiffuseMapSam;

//=============================================================================
// Task Shader - per-patch culling
//=============================================================================
groupshared TerrainPayload terrainPayload;

[numthreads(32, 1, 1)]
void TS(uint gid : SV_GroupID, uint gtid : SV_GroupIndex, uint dtid : SV_DispatchThreadID)
{
    uint visible = 0;

    if (dtid < gPatchCount)
    {
        // Frustum culling
        PatchMeta m = gPatchMeta[dtid];
        float3 mn = float3(m.worldX, m.minHeight, m.worldZ);
        float3 mx = float3(m.worldX + m.worldSize, m.minHeight + gMaxHeight, m.worldZ + m.worldSize);
        visible = AABBInFrustum(mn, mx, frustumPlanes);
    }

    // Compact visible patch indices into payload using WavePrefixCountBits
    if (visible)
    {
        uint index = WavePrefixSum(visible);
        terrainPayload.patchIndices[index] = dtid;
    }

    // Dispatch the required number of MS threadgroups to render the visible patches
    uint visibleCount = WaveActiveSum(visible);

    DispatchMesh(visibleCount, 1, 1, terrainPayload);
}

//=============================================================================
// Mesh Shader - 9x9 grid expansion (32 threads: each thread handles multiple vertices/indices via loop)
//=============================================================================

static const uint K_V  = 9;             // vertices per side
static const uint K_C  = K_V - 1;       // cells per side
static const uint K_VC = K_V * K_V;     // total vertices = 81
static const uint K_TC = K_C * K_C * 2; // total triangles = 128

struct VertexOutput
{
    float4 position    : SV_Position;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    float4 prevClipPos : TEXCOORD1;
};

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void MS(out indices uint3 triangles[K_TC],
        out vertices VertexOutput verts[K_VC],
        uint gtid : SV_GroupThreadID,
        uint gid : SV_GroupID)
{
    uint patchIndex = terrainPayload.patchIndices[gid];

    if (patchIndex >= gPatchCount)
    {
        return;
    }

    PatchMeta meta = gPatchMeta[patchIndex];

    // All threads must call SetMeshOutputCounts with the same values
    SetMeshOutputCounts(K_VC, K_TC);

    // ---- Vertex generation: each thread handles ceil(81/128)=1 vertices ----
    // Thread i processes vertices at indices: i, i+128, i+256 (if < 81)
    for (uint vi = gtid; vi < K_VC; vi += 128)
    {
        uint row = vi / K_V;
        uint col = vi % K_V;
        float u = (float)col / (float)(K_V - 1);
        float v = (float)row / (float)(K_V - 1);

        float worldX = meta.worldX + u * meta.worldSize;
        float worldZ = meta.worldZ + v * meta.worldSize;

        float texU = (worldX + _HalfWorldSize) / _WorldSize;
        float texV = (worldZ + _HalfWorldSize) / _WorldSize;
        float height = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, texV), 0);

        // 裂缝处理，取更粗patch边上上相邻两点的平均值作为新的高程
        uint nf = meta.neighborFlags;
        if (nf != 0)
        {
            if (col == 0 && (row & 1) && (nf & 1u))          // left(-X) coarser
            {
                float wz0 = meta.worldZ + (float)(row - 1) / (float)(K_V - 1) * meta.worldSize;
                float wz1 = meta.worldZ + (float)(row + 1) / (float)(K_V - 1) * meta.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, (wz0 + _HalfWorldSize) / _WorldSize), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, (wz1 + _HalfWorldSize) / _WorldSize), 0);
                height = (h0 + h1) * 0.5;
            }
            else if (col == K_V - 1 && (row & 1) && (nf & 2u)) // right(+X) coarser
            {
                float wz0 = meta.worldZ + (float)(row - 1) / (float)(K_V - 1) * meta.worldSize;
                float wz1 = meta.worldZ + (float)(row + 1) / (float)(K_V - 1) * meta.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, (wz0 + _HalfWorldSize) / _WorldSize), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, (wz1 + _HalfWorldSize) / _WorldSize), 0);
                height = (h0 + h1) * 0.5;
            }
            else if (row == K_V - 1 && (col & 1) && (nf & 4u)) // bottom(+Z) coarser
            {
                float wx0 = meta.worldX + (float)(col - 1) / (float)(K_V - 1) * meta.worldSize;
                float wx1 = meta.worldX + (float)(col + 1) / (float)(K_V - 1) * meta.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx0 + _HalfWorldSize) / _WorldSize, texV), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx1 + _HalfWorldSize) / _WorldSize, texV), 0);
                height = (h0 + h1) * 0.5;
            }
            else if (row == 0 && (col & 1) && (nf & 8u))      // top(-Z) coarser
            {
                float wx0 = meta.worldX + (float)(col - 1) / (float)(K_V - 1) * meta.worldSize;
                float wx1 = meta.worldX + (float)(col + 1) / (float)(K_V - 1) * meta.worldSize;
                float h0 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx0 + _HalfWorldSize) / _WorldSize, texV), 0);
                float h1 = gHeightmap.SampleLevel(gHeightmapSam, float2((wx1 + _HalfWorldSize) / _WorldSize, texV), 0);
                height = (h0 + h1) * 0.5;
            }
        }

        // Normal via finite differences
        float ts = 1.0 / (float)_GridSize;
        float hL = gHeightmap.SampleLevel(gHeightmapSam, float2(texU - ts, texV), 0);
        float hR = gHeightmap.SampleLevel(gHeightmapSam, float2(ts + texU, texV), 0);
        float hD = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, texV - ts), 0);
        float hU = gHeightmap.SampleLevel(gHeightmapSam, float2(texU, texV + ts), 0);

        float ws = _WorldSize / (float)_GridSize;
        float3 n = normalize(float3(
            -(hR - hL) / (2.0 * ws),
            1.0,
            -(hU - hD) / (2.0 * ws)
        ));
        float3 t = normalize(cross(float3(0, 1, 0), n));
        if (length(t) < 0.001)
            t = float3(1, 0, 0);

        float4 wp = float4(worldX, height, worldZ, 1.0);
        float4 cp = mul(wp, MATRIX_VP);
        float4 pp = mul(wp, MATRIX_PrevVP);
        float2 muv = float2(texU, texV) * _UVTileScale;

        verts[vi].position    = cp;
        verts[vi].normal      = n;
        verts[vi].tangent     = float4(t, 1.0);
        verts[vi].texCoord    = muv;
        verts[vi].prevClipPos = pp;
    }

    // ---- Index generation: each thread handles ceil(64/128)=1 cells ----
    // Thread i processes cells at indices: i, i+128 (if < 64)
    for (uint ci = gtid; ci < K_C * K_C; ci += 128)
    {
        //uint ci = gtid;
        uint r = ci / K_C;
        uint c = ci % K_C;
        uint v00 = r * K_V + c;
        uint v10 = v00 + 1;
        uint v01 = v00 + K_V;
        uint v11 = v01 + 1;

        triangles[ci * 2 + 0] = uint3(v00, v01, v11);
        triangles[ci * 2 + 1] = uint3(v00, v11, v10);
    }
}

//=============================================================================
// Pixel Shader - G-Buffer output
//=============================================================================

struct FragmentOutput
{
    float4 outRT0 : SV_TARGET0;
    float4 outRT1 : SV_TARGET1;
    float4 outRT2 : SV_TARGET2;
    float4 outRT3 : SV_TARGET3;
    float4 outRT4 : SV_TARGET4;
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);
    float4 baseColor = gDiffuseMap.Sample(gDiffuseMapSam, input.texCoord);

    FragmentOutput o;

    // RT0: (unused)
    o.outRT0 = float4(0.0, 0.0, 0.0, 1.0);

    // RT1: encoded normal
    normal = EncodeNormalOctahedron(normalize(normal));
    o.outRT1 = float4(normal, 0.5);

    // RT2: material flags
    uint lc = (10 << 4) | 1;
    o.outRT2 = float4(0.0, 0.5, 0.5, float(lc) / 255.0);

    // RT3: base color
    o.outRT3 = float4(baseColor.rgb, 1.0);

    // RT4: motion vectors
    float2 mv = float2(0.0, 0.0);
    if (input.prevClipPos.w != 0.0)
    {
        float2 cNDC = input.position.xy / input.position.w;
        float2 pNDC = input.prevClipPos.xy / input.prevClipPos.w;
        mv = cNDC - pNDC;
    }
    o.outRT4 = float4(mv, 0.0, 0.0);

    return o;
}

#endif
