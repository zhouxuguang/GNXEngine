#ifndef GNX_FULLSCREEN_TRIANGLE_HLSL
#define GNX_FULLSCREEN_TRIANGLE_HLSL

// Screen-space UVs follow the API texture convention: v = 0 is the top of the
// viewport, while NDC has +Y pointing up. Bridging the two therefore requires a
// Y flip, selected by TEXCOORD_FLIP (supplied by the shader compiler and always
// defined for the built-in pipeline). This helper is the single source of truth
// for that flip: the fullscreen-triangle UV generator and every shader that has
// to move between screen UV space and NDC (depth reconstruction, view-ray
// building, screen-space projection) call it instead of open-coding the #ifdef.
// The flip is its own inverse, so the same helper converts in both directions.
float2 fsScreenUVFlipY(float2 uv)
{
#ifdef TEXCOORD_FLIP
    uv.y = 1.0 - uv.y;
#endif
    return uv;
}

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
    return fsScreenUVFlipY(float2(u, v));
}

#endif
