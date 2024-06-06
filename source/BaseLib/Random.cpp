//
//  Random.cpp
//  BaseLib
//
//  Created by zhouxuguang on 15/1/5.
//  Copyright © 2017年 zhouxuguang. All rights reserved.
//

#include "Random.h"
#include <limits.h>
#include <unistd.h>

NS_BASELIB_BEGIN

unsigned int GetRandom()
{
    int nNum = 0;
#ifdef _MSC_VER
#if defined _WIN32_WCE
    CeGenRandom(sizeof(nNum), (PBYTE)&nNum);
#else
    HMODULE hLib = LoadLibraryA("ADVAPI32.DLL");
    if (hLib) {
        BOOLEAN (APIENTRY *pfn)(void*, ULONG) =
        (BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(hLib,"SystemFunction036");
        if (pfn) {
            ULONG ulCbBuff = sizeof(nNum);
            if(pfn(&nNum,ulCbBuff))
            {
            }
        }
        FreeLibrary(hLib);
    }
#endif
    
#elif defined __APPLE__
    return arc4random();
#elif defined __GNUC__
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd != -1)
    {
        (void) read(fd, (void *) &nNum, sizeof(int));
        (void) close(fd);
    }
    
#endif
    
    //最后的处理，苹果系统不需要
#if !defined(__APPLE__)
    return (unsigned int)((long long)nNum + (long long)INT_MAX + 1);
#endif
}

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

unsigned int GetRandom(unsigned int minNum, unsigned int maxNum)
{
    unsigned int minVal = std::min(minNum, maxNum);
    unsigned int maxVal = std::max(minNum, maxNum);
    return GetRandom() % (maxVal - minVal + 1) + minVal;
}

float GetRandom_0_1()
{
    return GetRandom() / (double)UINT_MAX;
}

float GetRandom_Minus1_1()
{
   return (GetRandom() / (double)UINT_MAX) * 2 - 1;
}

NS_BASELIB_END
