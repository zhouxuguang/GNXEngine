//
//  Terrain.shader
//  GNXEngine
//
//  Terrain G-buffer pass shader.
//  VS reads PatchMeta SSBO (via SV_InstanceID) and samples heightmap texture
//  to compute world-space vertex positions, normals, and tangents.
//  PS outputs the 5 G-buffer render targets for deferred lighting.
//

#ifndef GNX_ENGINE_TERRAIN_BASEPASS_HLSL
#define GNX_ENGINE_TERRAIN_BASEPASS_HLSL

#include "GNXEngineVariables.hlsl"
#include "GBufferCommon.hlsl"
#include "TerrainCommon.hlsl"

//=============================================================================
// Resources
//=============================================================================
StructuredBuffer<PatchMeta> gPatchMeta;
Texture2D<float> gHeightmap;
SamplerState gHeightmapSam;

// Material textures (currently only diffuse map)
Texture2D gDiffuseMap;
SamplerState gDiffuseMapSam;

//=============================================================================
// Vertex shader
//=============================================================================
struct VertexInput
{
    float3 position : POSITION;
    uint   instanceID : SV_InstanceID;
};

struct VertexOutput
{
    float4 position    : SV_POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    float4 prevClipPos : TEXCOORD1;
};

VertexOutput VS(VertexInput input)
{
    PatchMeta meta = gPatchMeta[input.instanceID];

    // Compute world XZ from template local position + PatchMeta
    float worldX = meta.worldX + input.position.x * meta.worldSize;
    float worldZ = meta.worldZ + input.position.z * meta.worldSize;

    // Sample heightmap: convert world XZ to UV [0,1]
    float u = (worldX + _HalfWorldSize) / _WorldSize;
    float v = (worldZ + _HalfWorldSize) / _WorldSize;
    float height = gHeightmap.SampleLevel(gHeightmapSam, float2(u, v), 0);
    float worldY = height;  // heightmap already stores world-space Y

    // Compute normal from heightmap finite differences
    float texelSize = 1.0 / (float)_GridSize;
    float hL = gHeightmap.SampleLevel(gHeightmapSam, float2(u - texelSize, v), 0);
    float hR = gHeightmap.SampleLevel(gHeightmapSam, float2(u + texelSize, v), 0);
    float hD = gHeightmap.SampleLevel(gHeightmapSam, float2(u, v - texelSize), 0);
    float hU = gHeightmap.SampleLevel(gHeightmapSam, float2(u, v + texelSize), 0);

    // World-space step between texels
    float worldStep = _WorldSize / (float)_GridSize;
    float3 computedNormal = normalize(float3(
        -(hR - hL) / (2.0 * worldStep),
        1.0,
        -(hU - hD) / (2.0 * worldStep)
    ));

    // Compute tangent (perpendicular to normal in the XZ plane)
    float3 computedTangent = normalize(cross(float3(0, 1, 0), computedNormal));
    // Fallback if cross product is zero (normal pointing straight up)
    if (length(computedTangent) < 0.001)
        computedTangent = float3(1, 0, 0);

    // Transform to clip space
    float4 worldPos = float4(worldX, worldY, worldZ, 1.0);
    float4 clipPos = mul(worldPos, MATRIX_VP);

    // Previous frame clip position (for motion vectors, terrain is static)
    float4 prevClipPos = mul(worldPos, MATRIX_PrevVP);

    // Material UV: Use the same mapping logic as heightmap for consistent texture mapping
    float2 materialUV = float2((worldX + _HalfWorldSize) / _WorldSize, 
                              (worldZ + _HalfWorldSize) / _WorldSize) * _UVTileScale;

    VertexOutput output;
    output.position    = clipPos;
    output.normal      = computedNormal;
    output.tangent     = float4(computedTangent, 1.0);
    output.texCoord    = materialUV;
    output.prevClipPos = prevClipPos;
    return output;
}

//=============================================================================
// Pixel shader
//=============================================================================
struct FragmentOutput
{
    float4 outRT0 : SV_TARGET0;   // Scene color (emissive)
    float4 outRT1 : SV_TARGET1;   // Normal + roughness
    float4 outRT2 : SV_TARGET2;   // Metallic + Specular + Roughness + ShadingModel
    float4 outRT3 : SV_TARGET3;   // BaseColor + AO
    float4 outRT4 : SV_TARGET4;   // Motion Vector
    float  outDepth : SV_Depth;   // 显式输出片元深度
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);

    // Sample diffuse texture only
    float4 baseColor = gDiffuseMap.Sample(gDiffuseMapSam, input.texCoord);

    // Use default material properties since we only have diffuse map
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float3 emissive = float3(0.0, 0.0, 0.0);

    FragmentOutput output;

    // RT0: Scene Color (emissive)
    output.outRT0 = float4(emissive.rgb, 1.0f);

    // RT1: Normal (octahedral encoded) + roughness
    normal = EncodeNormalOctahedron(normalize(normal));
    output.outRT1 = float4(normal, roughness);

    // RT2: Metallic + Specular + Roughness + ShadingModel
    uint lastComponent = (10 << 4) | (1);
    output.outRT2 = float4(metallic, 0.5f, roughness, float(lastComponent) / 255.0f);

    // RT3: BaseColor + AO
    output.outRT3 = float4(baseColor.rgb, ao);

    // RT4: Motion Vector (NDC-space offset)
    float2 motionVector = float2(0.0, 0.0);
    if (input.prevClipPos.w != 0.0)
    {
        float2 curNDC = input.position.xy / input.position.w;
        float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
        motionVector = curNDC - prevNDC;
    }
    output.outRT4 = float4(motionVector, 0.0, 0.0);

    // 显式输出片元深度（NDC 深度 = position.z / position.w）。
    // 参考 PPSSPP Vulkan 驱动 bug 文档：Adreno/Mali 对"光栅化写深度 vs 比深度"
    // 存在内部不一致时，shader 显式写 SV_Depth 可让驱动用确定性的深度参与测试，
    // 规避 PreDepth/GBuffer 两趟光栅化深度 1 ULP 差异导致的剔除/闪烁。
    output.outDepth = input.position.z / input.position.w;

    return output;
}

#endif // GNX_ENGINE_TERRAIN_BASEPASS_HLSL
