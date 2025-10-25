//
//  FastMathUtil.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/8.
//

#ifndef MATH3D_FASTMATH_UTIL_INCLUDE_H_HKD7FG
#define MATH3D_FASTMATH_UTIL_INCLUDE_H_HKD7FG

#include "Math3DCommon.h"

NS_MATHUTIL_BEGIN

class FastMathUtil
{
public:
    static float fastExp2(float x);
    
    static float fastLog2(float x);
    
    static float fastSinf(float x);
    
    static float fastCosf(float x);
    
    static float fastPowf(float base, float exponent);
    
private:
    FastMathUtil();
    FastMathUtil(const FastMathUtil&);
    ~FastMathUtil();
};

NS_MATHUTIL_END

#endif /* MATH3D_FASTMATH_UTIL_INCLUDE_H_HKD7FG */
