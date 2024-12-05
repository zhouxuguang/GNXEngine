//
//  SimdMath.h
//  GNXEditor
//
//  Created by zhouxuguang on 2023/9/29.
//

#ifndef GNXENGINE_SIMDMATH_INCLUDE_H
#define GNXENGINE_SIMDMATH_INCLUDE_H

#include "Math3DCommon.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

NS_MATHUTIL_BEGIN

#ifdef _MSC_VER
struct simd_float2
{
	float x;
	float y;
};

struct simd_float3
{
	float x;
	float y;
	float z;
	float pad;
};

struct simd_float4
{
	float x;
	float y;
	float z;
	float w;
};

#else
typedef float __attribute__((ext_vector_type(2))) simd_float2;

typedef float __attribute__((ext_vector_type(3))) simd_float3;

typedef float __attribute__((ext_vector_type(4))) simd_float4;
#endif

//构造simd类型的函数
simd_float2 make_simd_float2(float __x, float __y);

simd_float2 make_simd_float2(const Vector2f& vec);

simd_float3 make_simd_float3(float __x, float __y, float __z);

simd_float3 make_simd_float3(const Vector3f& vec);

simd_float4 make_simd_float4(float __x, float __y, float __z, float __w);

simd_float4 make_simd_float4(const Vector4f& vec);

NS_MATHUTIL_END

#endif /* GNXENGINE_SIMDMATH_INCLUDE_H */
