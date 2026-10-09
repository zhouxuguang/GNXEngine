//
//  FXAA.shader
//  GNXEngine
//
//  快速近似抗锯齿（Fast Approximate Anti-Aliasing）
//  参考 The-Modern-Vulkan-Cookbook 的 fxaa.frag 移植。
//
//  引擎的色调映射位于最后一个 Pass，这里直接采样场景 HDR 颜色，先做色调映射到
//  SDR 显示空间再执行 FXAA（亮度比较必须在显示空间进行），最后输出到交换链。
//

#include "GNXEngineCommon.hlsl"
#include "ToneMapping.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOut VS(uint vertexID : SV_VertexID)
{
    VertexOut vout;

    vout.PosH = fsTrianglePosition(vertexID);
    vout.texCoord = fsTriangleUV(vertexID);

    return vout;
}

Texture2D texImage;
SamplerState texImageSam;

#define FXAA_EDGE_THRESHOLD_MIN   (1.0 / 16.0)   // 亮度差低于此值判定为平坦区域
#define FXAA_EDGE_THRESHOLD_MAX   (1.0 / 8.0)    // 高亮区域的相对阈值上限，避免过检
#define FXAA_PIXEL_BLEND_LIMIT    (3.0 / 4.0)
#define FXAA_MIN_PIXEL_ALIASING   (1.0 / 8.0)
#define FXAA_SEARCH_STEPS         1

static const int FXAA_CENTER       = 0;
static const int FXAA_TOP          = 1;
static const int FXAA_BOTTOM       = 2;
static const int FXAA_LEFT         = 3;
static const int FXAA_RIGHT        = 4;
static const int FXAA_TOP_RIGHT    = 5;
static const int FXAA_BOTTOM_RIGHT = 6;
static const int FXAA_TOP_LEFT     = 7;
static const int FXAA_BOTTOM_LEFT  = 8;

static const float2 FXAA_OFFSETS[9] =
{
    float2( 0.0,  0.0), float2( 0.0, -1.0), float2( 0.0,  1.0),
    float2(-1.0,  0.0), float2( 1.0,  0.0), float2( 1.0, -1.0),
    float2( 1.0,  1.0), float2(-1.0, -1.0), float2(-1.0,  1.0)
};

float3 FxaaSample(float2 uv)
{
    return TonemapSceneColor(texImage.SampleLevel(texImageSam, uv, 0));
}

float FxaaSampleLuma(float2 uv)
{
    return RgbToLuma(FxaaSample(uv));
}

// 沿边缘方向搜索端点。返回是否需要做边缘 AA，并通过 outEdgeAntiAliasCoord 输出取色坐标。
float FindEdgeAntiAliasPixel(float2 texCoordMiddle,
                             float2 texelSize,
                             float lumaMiddle,
                             float lumaHighContrastPixel,
                             float stepLength,
                             bool isHorizontal,
                             out float2 outEdgeAntiAliasCoord)
{
    float2 highContrastCoord = texCoordMiddle;
    float2 edgeDir;

    if (isHorizontal)
    {
        highContrastCoord.y = texCoordMiddle.y + stepLength;
        edgeDir = float2(texelSize.x, 0.0);
    }
    else
    {
        highContrastCoord.x = texCoordMiddle.x + stepLength;
        edgeDir = float2(0.0, texelSize.y);
    }

    float2 posHighContrastNeg = highContrastCoord - edgeDir;
    float2 posHighContrastPos = highContrastCoord + edgeDir;
    float2 posMiddleNeg = texCoordMiddle - edgeDir;
    float2 posMiddlePos = texCoordMiddle + edgeDir;

    float lumaMiddleNegDir = 0.0;
    float lumaMiddlePosDir = 0.0;
    bool doneNegDir = false;
    bool donePosDir = false;

    for (int i = 0; i < FXAA_SEARCH_STEPS; ++i)
    {
        if (!doneNegDir)
        {
            const float lumaHighContrastNegDir = FxaaSampleLuma(posHighContrastNeg);
            lumaMiddleNegDir = FxaaSampleLuma(posMiddleNeg);
            doneNegDir = abs(lumaHighContrastNegDir - lumaHighContrastPixel) >
                             abs(lumaHighContrastNegDir - lumaMiddle) ||
                         abs(lumaMiddleNegDir - lumaMiddle) >
                             abs(lumaMiddleNegDir - lumaHighContrastPixel);
        }

        if (!donePosDir)
        {
            const float lumaHighContrastPosDir = FxaaSampleLuma(posHighContrastPos);
            lumaMiddlePosDir = FxaaSampleLuma(posMiddlePos);
            donePosDir = abs(lumaHighContrastPosDir - lumaHighContrastPixel) >
                             abs(lumaHighContrastPosDir - lumaMiddle) ||
                         abs(lumaMiddlePosDir - lumaMiddle) >
                             abs(lumaMiddlePosDir - lumaHighContrastPixel);
        }

        if (doneNegDir && donePosDir)
        {
            break;
        }

        if (!doneNegDir)
        {
            posHighContrastNeg -= edgeDir;
            posMiddleNeg -= edgeDir;
        }
        if (!donePosDir)
        {
            posHighContrastPos += edgeDir;
            posMiddlePos += edgeDir;
        }
    }

    float dstNeg;
    float dstPos;
    if (isHorizontal)
    {
        dstNeg = texCoordMiddle.x - posMiddleNeg.x;
        dstPos = posMiddlePos.x - texCoordMiddle.x;
    }
    else
    {
        dstNeg = texCoordMiddle.y - posMiddleNeg.y;
        dstPos = posMiddlePos.y - texCoordMiddle.y;
    }

    const bool closerToNegDir = dstNeg < dstPos;
    const float dst = min(dstNeg, dstPos);
    const float lumaEndPoint = closerToNegDir ? lumaMiddleNegDir : lumaMiddlePosDir;
    const bool edgeAARequired =
        abs(lumaEndPoint - lumaHighContrastPixel) < abs(lumaEndPoint - lumaMiddle);

    // pixelOffset = 0.5 时表示取色点落在边缘正中
    const float pixelOffset = dst * (-1.0 / (dstNeg + dstPos)) + 0.5;

    outEdgeAntiAliasCoord = texCoordMiddle;
    if (isHorizontal)
    {
        outEdgeAntiAliasCoord.y += pixelOffset * stepLength;
    }
    else
    {
        outEdgeAntiAliasCoord.x += pixelOffset * stepLength;
    }

    return edgeAARequired ? 1.0 : 0.0;
}

float4 PS(VertexOut pin) : SV_Target0
{
    uint width, height;
    texImage.GetDimensions(width, height);
    const float2 texelSize = float2(1.0 / (float)width, 1.0 / (float)height);
    const float2 texCoord = pin.texCoord;

    // 采样 3x3 邻域（同时色调映射），并用中心加四邻域粗判平坦区域
    float3 rgb[9];
    float luma[9];
    float3 rgbSum = float3(0.0, 0.0, 0.0);
    float lumaMin = 100000000.0;
    float lumaMax = 0.0;

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        rgb[i] = FxaaSample(texCoord + FXAA_OFFSETS[i] * texelSize);
        rgbSum += rgb[i];
        luma[i] = RgbToLuma(rgb[i]);

        if (i < 5)
        {
            lumaMin = min(lumaMin, luma[i]);
            lumaMax = max(lumaMax, luma[i]);
        }
    }

    const float lumaRange = lumaMax - lumaMin;
    if (lumaRange < max(FXAA_EDGE_THRESHOLD_MIN, FXAA_EDGE_THRESHOLD_MAX * lumaMax))
    {
        return float4(rgb[FXAA_CENTER], 1.0);
    }

    const float lumaTopBottom     = luma[FXAA_TOP] + luma[FXAA_BOTTOM];
    const float lumaLeftRight     = luma[FXAA_LEFT] + luma[FXAA_RIGHT];
    const float lumaTopCorners    = luma[FXAA_TOP_LEFT] + luma[FXAA_TOP_RIGHT];
    const float lumaBottomCorners = luma[FXAA_BOTTOM_LEFT] + luma[FXAA_BOTTOM_RIGHT];
    const float lumaLeftCorners   = luma[FXAA_TOP_LEFT] + luma[FXAA_BOTTOM_LEFT];
    const float lumaRightCorners  = luma[FXAA_TOP_RIGHT] + luma[FXAA_BOTTOM_RIGHT];

    // 像素混合量：与 3x3 平均色混合的比例，用于抑制细碎走样
    const float averageLumaTBLR = (lumaTopBottom + lumaLeftRight) * 0.25;
    const float lumaSubRange = abs(averageLumaTBLR - luma[FXAA_CENTER]);
    float pixelBlendAmount = max(0.0, (lumaSubRange / lumaRange) - FXAA_MIN_PIXEL_ALIASING);
    pixelBlendAmount = min(FXAA_PIXEL_BLEND_LIMIT,
                           pixelBlendAmount * (1.0 / (1.0 - FXAA_MIN_PIXEL_ALIASING)));

    const float3 averageRgbNeighbor = rgbSum * (1.0 / 9.0);

    // 判断边缘走向（水平 / 垂直）
    const float verticalEdgeRow1 = abs(-2.0 * luma[FXAA_TOP] + lumaTopCorners);
    const float verticalEdgeRow2 = abs(-2.0 * luma[FXAA_CENTER] + lumaLeftRight);
    const float verticalEdgeRow3 = abs(-2.0 * luma[FXAA_BOTTOM] + lumaBottomCorners);
    const float verticalEdge = (verticalEdgeRow1 + verticalEdgeRow2 * 2.0 + verticalEdgeRow3) / 12.0;

    const float horizontalEdgeCol1 = abs(-2.0 * luma[FXAA_LEFT] + lumaLeftCorners);
    const float horizontalEdgeCol2 = abs(-2.0 * luma[FXAA_CENTER] + lumaTopBottom);
    const float horizontalEdgeCol3 = abs(-2.0 * luma[FXAA_RIGHT] + lumaRightCorners);
    const float horizontalEdge = (horizontalEdgeCol1 + horizontalEdgeCol2 * 2.0 + horizontalEdgeCol3) / 12.0;

    const bool isHorizontal = horizontalEdge >= verticalEdge;

    // 取对比度更高的一侧，确定跨边缘的步进方向
    const float luma1 = isHorizontal ? luma[FXAA_TOP] : luma[FXAA_LEFT];
    const float luma2 = isHorizontal ? luma[FXAA_BOTTOM] : luma[FXAA_RIGHT];
    const bool is1Steepest = abs(luma[FXAA_CENTER] - luma1) >= abs(luma[FXAA_CENTER] - luma2);

    float stepLength = isHorizontal ? -texelSize.y : -texelSize.x;
    float lumaHighContrastPixel;
    if (is1Steepest)
    {
        lumaHighContrastPixel = luma1;
    }
    else
    {
        lumaHighContrastPixel = luma2;
        stepLength = -stepLength;
    }

    float2 edgeAntiAliasCoord;
    float3 edgeAntiAliasPixel = rgb[FXAA_CENTER];
    const float edgeAARequired = FindEdgeAntiAliasPixel(
        texCoord, texelSize, luma[FXAA_CENTER], lumaHighContrastPixel,
        stepLength, isHorizontal, edgeAntiAliasCoord);

    if (edgeAARequired >= 0.5)
    {
        edgeAntiAliasPixel = FxaaSample(edgeAntiAliasCoord);
    }

    return float4(lerp(edgeAntiAliasPixel, averageRgbNeighbor, pixelBlendAmount), 1.0);
}
