//
//  MatrixUtil.h
//  Math3D
//
//  Created by zhouxuguang on 16/9/24.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef MatrixUtil_hpp
#define MatrixUtil_hpp


#include "Math3DCommon.h"

#ifdef __SSE__
#include <xmmintrin.h>
#endif

NS_MATHUTIL_BEGIN

class MatrixUtil
{
private:
    static bool isNeon32Enabled();
    static bool isNeon64Enabled();
private:
#ifdef __SSE__
    static void addMatrix(const __m128 m[4], float scalar, __m128 dst[4]);
    
    static void addMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]);
    
    static void subtractMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]);
    
    static void multiplyMatrix(const __m128 m[4], float scalar, __m128 dst[4]);
    
    static void multiplyMatrix(const __m128 m1[4], const __m128 m2[4], __m128 dst[4]);
    
    static void negateMatrix(const __m128 m[4], __m128 dst[4]);
    
    static void transposeMatrix(const __m128 m[4], __m128 dst[4]);
    
    static void transformVec4(const __m128 m[4], const __m128& v, __m128& dst);
#endif
    static void addMatrix(const float* m, float scalar, float* dst);
    
    static void addMatrix(const float* m1, const float* m2, float* dst);
    
    static void subtractMatrix(const float* m1, const float* m2, float* dst);
    
    static void multiplyMatrix(const float* m, float scalar, float* dst);
    
    static void multiplyMatrix(const float* m1, const float* m2, float* dst);
    
    static void negateMatrix(const float* m, float* dst);
    
    static void transposeMatrix(const float* m, float* dst);
    
    static void transformVec4(const float* m, float x, float y, float z, float w, float* dst);
    
    static void transformVec4(const float* m, const float* v, float* dst);
    
    static void crossVec3(const float* v1, const float* v2, float* dst);
};

NS_MATHUTIL_END

#endif /* MatrixUtil_hpp */
