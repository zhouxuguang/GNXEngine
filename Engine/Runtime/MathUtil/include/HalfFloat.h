//
//  HalfFloat.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/19.
//

#ifndef GNXENGINE_HALFFLOAT_INCLDE_H
#define GNXENGINE_HALFFLOAT_INCLDE_H

#include "Math3DCommon.h"

NS_MATHUTIL_BEGIN

#define FP16_ONE     ((uint16_t) 0x3c00)
#define FP16_ZERO    ((uint16_t) 0)
#define FP16_NAGATIVE_ONE ((uint16_t) 0xbc00)

MATH3D_API float half_to_float(const uint16_t x);

MATH3D_API uint16_t float_to_half(const float x);

MATH3D_API bool half_is_negative(const uint16_t h);

NS_MATHUTIL_END

#endif /* GNXENGINE_HALFFLOAT_INCLDE_H */
