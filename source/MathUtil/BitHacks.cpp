//
//  BitHacks.cpp
//  Math3D
//
//  Created by zhouxuguang on 16/9/26.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "BitHacks.h"

NS_MATHUTIL_BEGIN

bool IsPowerOfTwo(unsigned int value)
{
    return (value > 0) && ((value & (value - 1)) == 0);
}

bool IsPowerOfTwo(int value)
{
    return (value > 0) && ((value & (value - 1)) == 0);
}

unsigned int Log2OfPowerOfTwo(unsigned int powerOfTwo)
{
    unsigned int log2 = (powerOfTwo & 0xAAAAAAAA) != 0;
    log2 |= ((powerOfTwo & 0xFFFF0000) != 0) << 4;
    log2 |= ((powerOfTwo & 0xFF00FF00) != 0) << 3;
    log2 |= ((powerOfTwo & 0xF0F0F0F0) != 0) << 2;
    log2 |= ((powerOfTwo & 0xCCCCCCCC) != 0) << 1;
    return log2;
}

int Log2OfPowerOfTwo(int powerOfTwo)
{
    unsigned int log2 = (powerOfTwo & 0xAAAAAAAA) != 0;
    log2 |= ((powerOfTwo & 0xFFFF0000) != 0) << 4;
    log2 |= ((powerOfTwo & 0xFF00FF00) != 0) << 3;
    log2 |= ((powerOfTwo & 0xF0F0F0F0) != 0) << 2;
    log2 |= ((powerOfTwo & 0xCCCCCCCC) != 0) << 1;
    return (int)log2;
}

bool IsOppositeSigns(int value1,int value2)
{
    return (value1 ^ value2) < 0;
}

unsigned int GetAbs(int value)
{
    unsigned int r;  // the result goes here
    int const mask = value >> (sizeof(int) * CHAR_BIT - 1);
    
    //r = (value + mask) ^ mask;
    //Patented variation:
    r = (value ^ mask) - mask;
    return r;
}

unsigned int NextPowerOfTwo(unsigned int value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    
    return value;
}

NS_MATHUTIL_END
