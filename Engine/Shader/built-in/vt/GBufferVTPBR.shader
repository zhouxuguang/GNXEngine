#ifndef GNX_ENGINE_GBUFFER_VT_PBR_HLSL
#define GNX_ENGINE_GBUFFER_VT_PBR_HLSL

#include "../GNXEngineVariables.hlsl"
#include "../GBufferCommon.hlsl"
#include "VirtualTextureCommon.hlsl"

// 虚拟纹理资源
Texture2D<uint> pageTable;
SamplerState pageTableSam;
Texture2D atlas;
SamplerState atlasSam;

// 常规材质贴图（normal / metalRough / ambient / emissive 仍用常规纹理）
Texture2D gNormalMap;
SamplerState gNormalMapSam;
Texture2D gMetalRoughMap;
SamplerState gMetalRoughMapSam;
Texture2D gAmbientMap;
SamplerState gAmbientMapSam;
Texture2D gEmissiveMap;
SamplerState gEmissiveMapSam;

// 虚拟纹理常量（布局必须与 C++ 侧 VTInfoBufferData 一致）
cbuffer cbVTInfo
{
    float2 pageGrid;      // mip0 的 tile 网格数 (virtualSize / pageSize)
    float2 tileSize;      // 单个 tile 的像素尺寸 (pageSize, pageSize)
    float2 atlasSize;     // 物理 atlas 的像素尺寸
    float2 virtualSize;   // 虚拟纹理像素尺寸
    float pagePadding;    // tile 四周 padding（像素）
    float slotSize;       // 物理 slot 步长 = pageSize + 2 * pagePadding
    float minMipLevel;    // 可采样的最小 mip
    float maxMipLevel;    // 可采样的最大 mip
}

// 顶点着色器输入
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position    : SV_POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    float4 prevClipPos : TEXCOORD1;  // 上一帧的裁剪坐标（用于Motion Vector）
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    
    // 将顶点位置从模型空间变换到裁剪空间
    float4 worldPos = mul(float4(input.position.xyz, 1.0), MATRIX_M);
    output.position = mul(worldPos, MATRIX_V);
    output.position = mul(output.position, MATRIX_P);

    // 计算上一帧的裁剪坐标（用于 Motion Vector）
    float4 prevWorldPos = worldPos;
    output.prevClipPos = mul(prevWorldPos, MATRIX_PrevVP);

    float3 normal = mul(float4(input.normal.xyz, 0.0f), MATRIX_Normal).xyz;
    output.normal = normal;
    
    float3 tangent = mul(float4(input.tangent.xyz, 0.0f), MATRIX_Normal).xyz;
    output.tangent.xyz = tangent;
    output.tangent.w = input.tangent.w;
    
    output.texCoord = input.texCoord;
    
    return output;
}

// 读取某一 mip 层级下、uv 对应的 page table entry。
// page table 与 feedback 使用同一个 Y 翻转约定，故此处也要翻转 Y。
// outGrid 返回该 mip 的 tile 网格数，供上层计算局部 UV。
uint FetchPageEntry(uint mip, float2 uv, out float2 outGrid)
{
    float2 grid = max(pageGrid * exp2(-float(mip)), float2(1.0, 1.0));
    outGrid = grid;

    float2 pageCoords = floor(uv * grid);
    pageCoords.y = (grid.y - 1.0) - pageCoords.y;
    pageCoords = clamp(pageCoords, float2(0.0, 0.0), grid - 1.0);

    // 采样该 mip 的 texel 中心（page table 使用 point 采样器）
    float2 texelUV = (pageCoords + 0.5) / grid;
    return pageTable.SampleLevel(pageTableSam, texelUV, float(mip)).r;
}

// 通过 page table 采样虚拟纹理：从屏幕足迹决定的 mip 开始，逐级向粗糙 mip 回退，
// 直到找到 resident 的 page。找到后按 padding 计算物理 atlas UV，并用显式梯度采样。
float4 SampleVirtualTexture(float2 uv)
{
    float mipFloat = ComputeMipLevel(uv.x, uv.y, virtualSize.x, virtualSize.y);
    uint startMip = uint(clamp(mipFloat, minMipLevel, maxMipLevel));
    uint endMip   = uint(maxMipLevel);

    uint entry = 0u;
    bool resident = false;
    float2 activeGrid = max(pageGrid, float2(1.0, 1.0));

    for (uint mip = startMip; mip <= endMip; ++mip)
    {
        float2 grid;
        uint e = FetchPageEntry(mip, uv, grid);
        if ((e & 1u) != 0u)
        {
            entry = e;
            activeGrid = grid;
            resident = true;
            break;
        }
    }

    if (!resident)
    {
        // 兜底：常驻 mip 尚未就绪时显示黑色
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    // C++ 编码: bit0=resident, bit1-8=slotX, bit9-16=slotY
    float2 physicalSlot = float2((entry >> 1) & 0xFFu, (entry >> 9) & 0xFFu);
    float2 localUV = frac(uv * activeGrid);

    // 物理采样位置 = slot 原点 + padding + tile 内偏移
    float2 sampleTexel = physicalSlot * slotSize + pagePadding + localUV * tileSize;
    float2 atlasUV = sampleTexel / atlasSize;

    // atlasUV 在 tile 边界不连续，自动 LOD 会选错，必须用显式梯度
    float2 dx = ddx(uv) * activeGrid * (tileSize / atlasSize);
    float2 dy = ddy(uv) * activeGrid * (tileSize / atlasSize);

    return atlas.SampleGrad(atlasSam, atlasUV, dx, dy);
}

struct FragmentOutput
{
    float4 outRT0 : SV_TARGET0;   //scene color
    float4 outRT1 : SV_TARGET1;   //Normal + 0.33333f
    float4 outRT2 : SV_TARGET2;   //Metallic + Specular + Roughness + [4 bit 0b1010 | 4 bit ShadingModel]
    float4 outRT3 : SV_TARGET3;   //BaseColor + GenericAO
    float4 outRT4 : SV_TARGET4;   //Motion Vector (NDC空间偏移，RG通道)
};

FragmentOutput PS(VertexOutput input)
{
    float3 normal = normalize(input.normal);
    float4 tangent = float4(normalize(input.tangent.xyz), input.tangent.w);
    
    // 采样法线贴图（如果有）
    float3 normalTS = gNormalMap.Sample(gNormalMapSam, input.texCoord).xyz;
    normalTS = normalTS * 2.0 - 1.0;  // [0,1] -> [-1,1]
    normal = NormalSampleToWorldSpace(normalTS, normal, tangent);
    
    // 通过虚拟纹理采样 base color（替代 gDiffuseMap）
    float4 baseColor = SampleVirtualTexture(input.texCoord);
    
    // 其余材质贴图仍使用常规纹理
    float4 metalRough = gMetalRoughMap.Sample(gMetalRoughMapSam, input.texCoord);
    float ao = gAmbientMap.Sample(gAmbientMapSam, input.texCoord).r;
    float3 emissive = gEmissiveMap.Sample(gEmissiveMapSam, input.texCoord).rgb;

    // glTF metallic-roughness 约定：G = roughness，B = metallic
    float metallic = metalRough.b;
    float perceptualRoughness = metalRough.g;
    
    FragmentOutput output;

    // RT0: Scene Color (写入自发光颜色，延迟渲染阶段会叠加光照)
    output.outRT0 = float4(emissive.rgb, 1.0f);

    // 法线编码
    normal = EncodeNormalOctahedron(normalize(normal));
    output.outRT1 = float4(normal, 0.333333f);

    // RT2: Metallic + Specular(0.5) + Roughness(g通道) + [4 bit 0b1010 | 4 bit ShadingModel]
    uint lastCompoent = (10 << 4) | (1);
    output.outRT2 = float4(metallic, 0.5f, perceptualRoughness, float(lastCompoent) / 255.0f);

    // RT3: BaseColor + AO
    output.outRT3 = float4(baseColor.rgb, ao);

    // RT4: Motion Vector（NDC 空间偏移）
    float2 motionVector = float2(0.0, 0.0);
    if (input.prevClipPos.w != 0.0)
    {
        float2 curNDC = input.position.xy / input.position.w;
        float2 prevNDC = input.prevClipPos.xy / input.prevClipPos.w;
        motionVector = curNDC - prevNDC;
    }
    output.outRT4 = float4(motionVector, 0.0, 0.0);

    return output;
}

#endif