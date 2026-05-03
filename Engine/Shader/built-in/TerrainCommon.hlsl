#ifndef GNXENGINE_TERRAINCOMMON_HLSL
#define GNXENGINE_TERRAINCOMMON_HLSL

//=============================================================================
// PatchMeta (must match C++ QuadTreeTerrain::PatchMeta exactly)
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

cbuffer cbTerrain
{
    float _WorldSize;
    float _HalfWorldSize;
    float _UVTileScale;
    uint  _GridSize;
};

cbuffer cbTerrainCull
{
    uint  gPatchCount;
    float gMaxHeight;
    uint  _pad0;
    uint  _pad1;
};

//=============================================================================
// Frustum culling (shared logic with TerrainCull.shader)
//=============================================================================
void ExtractFrustumPlanes(float4x4 vp, out float4 planes[6])
{
    planes[0] = vp[3] + vp[0]; // Left
    planes[1] = vp[3] - vp[0]; // Right
    planes[2] = vp[3] + vp[1]; // Bottom
    planes[3] = vp[3] - vp[1]; // Top
    planes[4] = vp[2];         // Near  (DirectX/Metal: 0 <= z <= w)
    planes[5] = vp[3] - vp[2]; // Far

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float len = length(planes[i].xyz);
        planes[i] /= len;
    }
}

bool AABBInFrustum(float3 aabbMin, float3 aabbMax, float4 planes[6])
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
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

#endif //GNXENGINE_TERRAINCOMMON_HLSL