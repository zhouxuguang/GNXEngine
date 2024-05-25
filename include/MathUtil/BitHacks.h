//
//  BitHacks.h
//  Math3D
//
//  Created by zhouxuguang on 16/9/26.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef MATH3D_BITHACKS_INCLUDE_H_HKDF987FG
#define MATH3D_BITHACKS_INCLUDE_H_HKDF987FG

#include "Math3DCommon.h"

//参考http://graphics.stanford.edu/~seander/bithacks.html

NS_MATHUTIL_BEGIN

//判断该整数是否是2的整数次方
MATH3D_API bool IsPowerOfTwo(unsigned int value);
MATH3D_API bool IsPowerOfTwo(int value);

//求以2为底的结果
MATH3D_API unsigned int Log2OfPowerOfTwo(unsigned int powerOfTwo);
MATH3D_API int Log2OfPowerOfTwo(int powerOfTwo);

//判断两个数是否异号
MATH3D_API bool IsOppositeSigns(int value1,int value2);

//不使用分支计算绝对值
MATH3D_API unsigned int GetAbs(int value);

//大于等于value的最近的二的次方的数
MATH3D_API unsigned int NextPowerOfTwo(unsigned int value);


NS_MATHUTIL_END

#endif /* MATH3D_BITHACKS_INCLUDE_H_HKDF987FG */
