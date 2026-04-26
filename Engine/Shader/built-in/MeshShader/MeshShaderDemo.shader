// MeshShaderDemo.shader
// Demonstrates basic mesh shader usage: task shader dispatches 3 mesh workgroups,
// each mesh workgroup generates 1 triangle offset in Z, resulting in 3 overlapping
// colored triangles (green, blue, red vertices).

// ==================== UBO ====================
cbuffer UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};

// ==================== Task Shader (Amplification Shader) ====================
struct DummyPayLoad
{
    uint dummyData;
};

groupshared DummyPayLoad dummyPayLoad;

[numthreads(1, 1, 1)]
void AS()
{
    DispatchMesh(2, 1, 1, dummyPayLoad);
}

// ==================== Mesh Shader ====================
struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

static const float4 positions[3] = {
    float4( -0.5, 1.0, 0.0, 1.0),
    float4(-1.0,  -1.0, 0.0, 1.0),
    float4( 0.0,  -1.0, 0.0, 1.0)
};

static const float4 colors[3] = {
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
    float4(1.0, 0.0, 0.0, 1.0)
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void MS(out indices uint3 triangles[1], out vertices VertexOutput vertices[3],
        uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float4x4 mvp = mul(projection, mul(view, model));
    float offsetX = 0.0;
    if (DispatchThreadID.x > 0) 
    {
        offsetX = 1.0;
    }
    float4 offset = float4((float)DispatchThreadID.x, 0.0, 0.0, 0.0);
    SetMeshOutputCounts(3, 1);

    for (uint i = 0; i < 3; i++)
    {
        vertices[i].position = mul(mvp, positions[i] + offset);
        vertices[i].color = colors[i];
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
