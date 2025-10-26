//
//  FastMathUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/8.
//

#include "FastMathUtil.h"

//https://github.com/etheory/fastapprox  这里据说是更快速的实现

NS_MATHUTIL_BEGIN

#define _BIT(n) (1<<(n))
float FastMathUtil::fastExp2(float x)
{
    union { uint32_t i; float f; } v;
    float offset = (x < 0) ? 1.0f : 0.0f;
    float clipp = (x < -126) ? -126.0f : x;
    int w = clipp;
    float z = clipp - w + offset;
    v.i = (uint32_t)(_BIT(23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z));
    return v.f;
}

float FastMathUtil::fastLog2(float x)
{
    union { float f; uint32_t i; } vx;
    union { uint32_t i; float f; } mx;
    vx.f = x;
    mx.i = (vx.i & 0x007FFFFF) | 0x3F000000;
    float y = vx.i;
    y *= 1.1920928955078125e-7f;
    return y - 124.22551499f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f);
}

#define M_TAU (2*M_PI)
static float fast_cosc(float x)
{
    x -= 0.25f + floorf(x + 0.25f);
    x *= 16.0f * (fabs(x) - 0.5f);
    x += 0.225f * x * (fabs(x) - 1.0f);
    return x;
}

float FastMathUtil::fastSinf(float x)
{
    return fast_cosc(x/M_TAU - 0.25f);
}

float FastMathUtil::fastCosf(float x)
{
    return fast_cosc(x/M_TAU);
}

float FastMathUtil::fastPowf(float base, float exponent)
{
    return fastExp2(fastLog2(base)*exponent);
}

NS_MATHUTIL_END
