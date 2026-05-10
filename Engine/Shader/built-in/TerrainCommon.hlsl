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
    uint  neighborFlags;
    uint _pad0;
    uint _pad1;
    uint _pad2;
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

#define AS_GROUP_SIZE 1024
struct TerrainPayload
{
    uint patchIndices[AS_GROUP_SIZE];
};

bool AABBInFrustum(float3 aabbMin, float3 aabbMax, float4 planes[6])
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // p-vertex: 选沿平面法线方向最远的AABB角点（最可能在视锥体内）
        float3 p = float3(
            planes[i].x > 0 ? aabbMax.x : aabbMin.x,
            planes[i].y > 0 ? aabbMax.y : aabbMin.y,
            planes[i].z > 0 ? aabbMax.z : aabbMin.z
        );
        if (dot(planes[i].xyz, p) + planes[i].w < 0)
            return false;
    }
    return true;
}

#endif //GNXENGINE_TERRAINCOMMON_HLSL