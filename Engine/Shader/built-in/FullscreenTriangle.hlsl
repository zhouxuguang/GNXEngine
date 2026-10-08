#ifndef GNX_FULLSCREEN_TRIANGLE_HLSL
#define GNX_FULLSCREEN_TRIANGLE_HLSL

// A three-vertex fullscreen triangle. UVs intentionally extend to 2 so that
// interpolation covers the viewport without a diagonal seam.
float4 fsTrianglePosition(int vertexID)
{
    float x = -1.0 + float((vertexID & 1) << 2);
    float y = -1.0 + float((vertexID & 2) << 1);
    return float4(x, y, 0.0, 1.0);
}

float2 fsTriangleUV(int vertexID)
{
    float u = vertexID == 1 ? 2.0 : 0.0;
    float v = vertexID == 2 ? 2.0 : 0.0;
#ifdef TEXCOORD_FLIP
    v = 1.0 - v;
#endif
    return float2(u, v);
}

#endif
