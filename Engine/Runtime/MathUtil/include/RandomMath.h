//
//  RandomMath.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/10/21.
//

#ifndef MATHUTIL_RANDOMMATH_INCLUDE_H
#define MATHUTIL_RANDOMMATH_INCLUDE_H

#include "Math3DCommon.h"
#include "Runtime/BaseLib/include/Random.h"
#include "Vector3.h"
#include "Vector2.h"

NS_MATHUTIL_BEGIN

MATH3D_API inline Vector3f RandomUnitVector3();

MATH3D_API inline Vector2f RandomUnitVector2();

NS_MATHUTIL_END

#endif /* MATHUTIL_RANDOMMATH_INCLUDE_H */
