// MeshShaderDemo.shader
// Demonstrates mesh shader reading vertex data from StructuredBuffer (SSBO).
// Task shader dispatches 2 mesh workgroups, each mesh workgroup reads vertices
// from gVertices SSBO and outputs 1 triangle.
//
// This test validates:
//   - MS + SSBO (StructuredBuffer) read path
//   - SetStorageBuffer(ShaderStage_Mesh) binding
//   - Basic indirect-ready command pattern

// ==================== UBO ====================
cbuffer UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};

// ==================== SSBO: Vertex Data (filled by CPU) ====================
struct VertexData
{
    float4 position;  // object-space position
    float4 color;     // vertex color
};

// Each triangle uses 3 consecutive vertices from the buffer.
// Triangle 0: vertices[0..2], Triangle 1: vertices[3..5]
StructuredBuffer<VertexData> gVertices;

// Number of vertices per triangle (fixed for this demo)
static const uint VERTS_PER_TRIANGLE = 3;

// ==================== Task Shader ====================
struct MeshPayload
{
    uint triangleIndex;  // which triangle this mesh group will output
};

groupshared MeshPayload payload;

[numthreads(1, 1, 1)]
void TS(in uint groupId : SV_GroupID)
{
    // Dispatch one mesh workgroup per triangle
    // The demo has 2 triangles in the vertex buffer
    payload.triangleIndex = groupId;
    DispatchMesh(1, 1, 1, payload);
}

// ==================== Mesh Shader ====================
struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void MS(out indices uint3 triangles[1], out vertices VertexOutput vertices[3], uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float4x4 mvp = mul(projection, mul(view, model));

    uint triIdx = payload.triangleIndex;
    uint baseVertex = triIdx * VERTS_PER_TRIANGLE;

    // Output 3 vertices from SSBO
    SetMeshOutputCounts(VERTS_PER_TRIANGLE, 1);

    for (uint i = 0; i < VERTS_PER_TRIANGLE; i++)
    {
        VertexData v = gVertices[baseVertex + i];
        vertices[i].position = mul(mvp, v.position);
        vertices[i].color = v.color;
    }

    triangles[0] = uint3(0, 1, 2);
}

// ==================== Fragment Shader ====================
struct PSInput
{
    float4 color : COLOR0;
};

float4 PS(PSInput input) : SV_Target
{
    return input.color;
}
