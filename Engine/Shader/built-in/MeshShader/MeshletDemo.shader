#ifndef MESHLET_DEMO_SHADER_H
#define MESHLET_DEMO_SHADER_H

#include "../GNXEngineVariables.hlsl"
#include "../Culling.hlsl"

cbuffer cbMeshletParams
{
    uint gInstanceCount;
    uint gMeshletCount;
};

struct Vertex 
{
    float Position[3];
};

struct Meshlet 
{
	uint VertexOffset;
	uint TriangleOffset;
	uint VertexCount;
	uint TriangleCount;
    float4 BoundingSphere;
};

StructuredBuffer<Vertex>  Vertices;
StructuredBuffer<Meshlet> Meshlets;
StructuredBuffer<uint>    VertexIndices;
StructuredBuffer<uint>    TriangleIndices;

struct Instance
{
    float4x4 M;
};
StructuredBuffer<Instance> Instances;

struct MeshOutput 
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

#define AS_GROUP_SIZE 32

struct Payload 
{
    uint InstanceIndices[AS_GROUP_SIZE];
    uint MeshletIndices[AS_GROUP_SIZE];
};

groupshared Payload sPayload;

[numthreads(AS_GROUP_SIZE, 1, 1)]
void TS(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid  : SV_GroupID
)
{
    bool visible = false;

    uint instanceIndex = dtid / gMeshletCount;
    uint meshletIndex  = dtid % gMeshletCount;

    if ((instanceIndex < gInstanceCount) && (meshletIndex < gMeshletCount)) 
    {
        float4x4 M = Instances[instanceIndex].M;
        float4 meshletBoundingSphere = mul(float4(Meshlets[meshletIndex].BoundingSphere.xyz, 1.0), M);
        meshletBoundingSphere.w = Meshlets[meshletIndex].BoundingSphere.w;

        visible = SphereInFrustum(meshletBoundingSphere.xyz, meshletBoundingSphere.w, frustumPlanes);
        // visible = true;
    }

    if (visible) 
    {
        uint index = WavePrefixCountBits(visible);
        sPayload.InstanceIndices[index] = instanceIndex;
        sPayload.MeshletIndices[index]  = meshletIndex;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, sPayload);
}

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void MS(uint gtid : SV_GroupThreadID, 
    uint gid : SV_GroupID, 
    in payload Payload payload,
    out indices uint3 triangles[128], 
    out vertices MeshOutput vertices[64]) 
{
    // 这里gid相当于只是一个索引，mesh shader中一个work group 对应一个meshlet
    uint meshletIndex = payload.MeshletIndices[gid];
    uint instanceIndex = payload.InstanceIndices[gid];

    Meshlet m = Meshlets[meshletIndex];

    SetMeshOutputCounts(m.VertexCount, m.TriangleCount);
       
    if (gtid < m.TriangleCount) 
    {
        //
        // meshopt stores the triangle offset in bytes since it stores the
        // triangle indices as 3 consecutive bytes. 
        //
        // Since we repacked those 3 bytes to a 32-bit uint, our offset is now
        // aligned to 4 and we can easily grab it as a uint without any 
        // additional offset math.
        //
        uint packed = TriangleIndices[m.TriangleOffset + gtid];
        uint vIdx0  = (packed >>  0) & 0xFF;
        uint vIdx1  = (packed >>  8) & 0xFF;
        uint vIdx2  = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(vIdx0, vIdx1, vIdx2);
    }

    if (gtid < m.VertexCount) 
    {
        uint vertexIndex = m.VertexOffset + gtid;        
        vertexIndex = VertexIndices[vertexIndex];

        // Transform to world space.
        float4 posW = mul(float4(Vertices[vertexIndex].Position[0], 
            Vertices[vertexIndex].Position[1], 
            Vertices[vertexIndex].Position[2], 1.0), Instances[instanceIndex].M);

        posW = mul(posW, MATRIX_V);
        posW = mul(posW, MATRIX_P);

        vertices[gtid].Position = posW;
        
        float3 color = float3(
            float(meshletIndex & 1),
            float(meshletIndex & 3) / 4,
            float(meshletIndex & 7) / 8);
        vertices[gtid].Color = color;
    }
}

float4 PS(MeshOutput input) : SV_TARGET
{
    //return float4(1.0, 0.0, 0.0, 1);
    return float4(input.Color, 1);
}

#endif