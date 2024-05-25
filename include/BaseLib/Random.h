//
//  Random.h
//  BaseLib 跨平台随机数生成器
//
//  Created by zhouxuguang on 15/1/5.
//  Copyright © 2017年 zhouxuguang. All rights reserved.
//

#ifndef OGSE_BASELIB_RANDOM_INCLUDE_H_87GDKL87
#define OGSE_BASELIB_RANDOM_INCLUDE_H_87GDKL87

#include "PreCompile.h"

NS_BASELIB_BEGIN

//获得随机数，不需要设置种子，随机数的范围是int值的范围
BASELIB_API unsigned int GetRandom();

//获得minNum-maxNum之间的随机数，不需要设置种子
BASELIB_API unsigned int GetRandom(unsigned int minNum,unsigned int maxNum);

//获得0-1之间的随机数，不需要设置种子
BASELIB_API float GetRandom_0_1();

//获得-1-1之间的随机数，不需要设置种子
BASELIB_API float GetRandom_Minus1_1();

NS_BASELIB_END


#endif /* OGSE_BASELIB_RANDOM_INCLUDE_H_87GDKL87 */
