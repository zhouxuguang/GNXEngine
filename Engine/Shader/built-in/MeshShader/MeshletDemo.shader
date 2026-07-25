#ifndef MESHLET_DEMO_SHADER_H
#define MESHLET_DEMO_SHADER_H

#include "../GNXEngineVariables.hlsl"
#include "../Culling.hlsl"

cbuffer cbMeshletParams
{
    uint4 gLODMeshletOffsets[5]; // [lodCount] 每个 LOD 的第一个 meshlet 索引
    uint4 gLODMeshletCounts[5]; // [lodCount] 每个 LOD 的 meshlet 数量
    uint gInstanceCount;
    uint gMeshletCount;
    uint gNumPartitions;
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
StructuredBuffer<uint>    MeshletPartitions; // partitionId per meshlet (METIS cluster group)

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
    // MUST be uint, NOT bool! SPIRV-Cross only pattern-matches
    // WavePrefixCountBits/WaveActiveCountBits → simd_prefix_exclusive_sum/simd_sum
    // when the operand is an integer type (OpTypeInt), not a boolean (OpTypeBool).
    // bool visible generates mismatched SPIR-V instructions that fail pattern matching,
    // resulting in broken MSL code.
    uint visible = 0;

    uint instanceIndex = dtid / gMeshletCount;
    uint meshletIndex  = dtid % gMeshletCount;
    
    if (instanceIndex < gInstanceCount) 
    {
        uint lod             = instanceIndex;
        uint lodMeshletCount = gLODMeshletCounts[lod].x;

        if (meshletIndex < lodMeshletCount)
        {
            meshletIndex += gLODMeshletOffsets[lod].x;

            float4x4 M = Instances[instanceIndex].M;
            float4 meshletBoundingSphere = mul(float4(Meshlets[meshletIndex].BoundingSphere.xyz, 1.0), M);
            meshletBoundingSphere.w = Meshlets[meshletIndex].BoundingSphere.w;

            visible = SphereInFrustum(meshletBoundingSphere.xyz, meshletBoundingSphere.w, frustumPlanes) ? 1 : 0;
        }
    }

    if (visible) 
    {
        // Use WavePrefixSum/WaveActiveSum instead of WavePrefixCountBits/WaveActiveCountBits.
        // WavePrefixCountBits generates OpGroupNonUniformBallot + OpGroupNonUniformBallotBitCount
        // in SPIR-V, which SPIRV-Cross translates to spvSubgroupBallot + spvSubgroupBallotExclusiveBitCount
        // (with threadgroup_barrier!). WavePrefixSum generates OpGroupNonUniformIAdd(ExclusiveScan),
        // which maps cleanly to simd_prefix_exclusive_sum in Metal without any ballot emulation.
        // Since visible is 0 or 1, prefix sum == prefix count of bits.
        uint index = WavePrefixSum(visible);
        sPayload.InstanceIndices[gtid] = instanceIndex;
        sPayload.MeshletIndices[gtid]  = meshletIndex;
    }

    // WaveActiveSum(visible): since visible is 0 or 1, sum == count
    uint visibleCount = WaveActiveSum(visible);
    DispatchMesh(AS_GROUP_SIZE, 1, 1, sPayload);
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

        float3 color;
        if (gNumPartitions > 0)
        {
            // 根据 METIS 分区 ID 着色，不同 cluster group 不同颜色
            uint partitionId = MeshletPartitions[meshletIndex];
            float hue = frac(float(partitionId) * 0.618033988749895); // golden ratio conjugate for good color distribution
            // HSV -> RGB 简化版 (S=0.8, V=0.9)
            float h = hue * 6.0;
            float c = 0.8 * 0.9;
            float x = c * (1.0 - abs(fmod(h, 2.0) - 1.0));
            float m = 0.9 - c;
            float3 rgb;
            if (h < 1.0)      rgb = float3(c, x, 0.0);
            else if (h < 2.0) rgb = float3(x, c, 0.0);
            else if (h < 3.0) rgb = float3(0.0, c, x);
            else if (h < 4.0) rgb = float3(0.0, x, c);
            else if (h < 5.0) rgb = float3(x, 0.0, c);
            else              rgb = float3(c, 0.0, x);
            color = rgb + m;
        }
        else
        {
            // 无分区数据时的回退着色
            color = float3(
                float(meshletIndex & 1),
                float(meshletIndex & 3) / 4,
                float(meshletIndex & 7) / 8);
        }
        vertices[gtid].Color = color;
    }
}

float4 PS(MeshOutput input) : SV_TARGET
{
    //return float4(1.0, 0.0, 0.0, 1);
    return float4(input.Color, 1);
}

#endif