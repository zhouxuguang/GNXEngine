#ifndef GNXENGINE_TERRAINCOMMON_HLSL
#define GNXENGINE_TERRAINCOMMON_HLSL

#include "Culling.hlsl"

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

#endif //GNXENGINE_TERRAINCOMMON_HLSL