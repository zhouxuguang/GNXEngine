//
//  RandomMath.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/21.
//

#ifndef MATHUTIL_RANDOMMATH_INCLUDE_H
#define MATHUTIL_RANDOMMATH_INCLUDE_H

#include "Math3DCommon.h"
#include "Random.h"
#include "Vector3.h"
#include "Vector2.h"
#include <math.h>

NS_MATHUTIL_BEGIN

inline Vector3f RandomUnitVector3()
{
    float z = baselib::GetRandom_Minus1_1();
    float a = baselib::GetRandom_0_1() * 2.0F * M_PI;

    float r = sqrtf(1.0f - z*z);

    float x = r * cos(a);
    float y = r * sin(a);

    return Vector3f(x, y, z);
}

inline Vector2f RandomUnitVector2()
{
    float a = baselib::GetRandom_0_1() * 2.0F * M_PI;

    float x = cos(a);
    float y = sin(a);

    return Vector2f(x, y);
}

NS_MATHUTIL_END

#endif /* MATHUTIL_RANDOMMATH_INCLUDE_H */
