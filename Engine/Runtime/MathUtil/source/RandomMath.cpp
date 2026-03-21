//
//  RandomMath.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/21.
//

#include "RandomMath.h"

NS_MATHUTIL_BEGIN

Vector3f RandomUnitVector3()
{
    float z = baselib::GetRandom_Minus1_1();
    float a = baselib::GetRandom_0_1() * 2.0F * M_PI;

    float r = sqrtf(1.0f - z * z);

    float x = r * cos(a);
    float y = r * sin(a);

    return Vector3f(x, y, z);
}

Vector2f RandomUnitVector2()
{
    float a = baselib::GetRandom_0_1() * 2.0F * M_PI;

    float x = cos(a);
    float y = sin(a);

    return Vector2f(x, y);
}

Vector3f UniformHemisphere()
{
    Vector3f result;
    float x1 = baselib::GetRandom_0_1();
    float x2 = baselib::GetRandom_0_1();
    float s = sqrt(1.0f - x1 * x1);
    result.x = cos(M_PI * 2.0f * x2) * s;
    result.y = sin(M_PI * 2.0f * x2) * s;
    result.z = x1;
    return result;
}

Vector3f UniformCircle()
{
    Vector3f result;
    float x = baselib::GetRandom_0_1();
    float angle = M_PI * 2.0f * x;
    result.x = cos(angle);
    result.y = sin(angle);
    result.z = 0.0f;
    return result;
}

NS_MATHUTIL_END
