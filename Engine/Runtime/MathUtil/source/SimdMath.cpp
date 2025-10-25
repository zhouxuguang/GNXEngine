//
//  SimdMath.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2023/9/29.
//

#include "SimdMath.h"

NS_MATHUTIL_BEGIN

simd_float2 make_simd_float2(float __x, float __y)
{
    simd_float2 vector;
    vector.x = __x;
    vector.y = __y;
    return vector;
}

simd_float2 make_simd_float2(const Vector2f& vec)
{
    simd_float2 vector;
    vector.x = vec.x;
    vector.y = vec.y;
    return vector;
}


simd_float3 make_simd_float3(float __x, float __y, float __z)
{
    simd_float3 vector;
    vector.x = __x;
    vector.y = __y;
    vector.z = __z;
    return vector;
}

simd_float3 make_simd_float3(const Vector3f& vec)
{
    simd_float3 vector;
    vector.x = vec.x;
    vector.y = vec.y;
    vector.z = vec.z;
    return vector;
}

simd_float4 make_simd_float4(float __x, float __y, float __z, float __w)
{
    simd_float4 vector;
    vector.x = __x;
    vector.y = __y;
    vector.z = __z;
    vector.z = __w;
    return vector;
}

simd_float4 make_simd_float4(const Vector4f& vec)
{
    simd_float4 vector;
    vector.x = vec.x;
    vector.y = vec.y;
    vector.z = vec.z;
    vector.z = vec.w;
    return vector;
}

NS_MATHUTIL_END
