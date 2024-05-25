//
//  DateTime.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/3.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "DateTime.h"

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif


NS_BASELIB_BEGIN

#ifndef WIN32

#include <sys/time.h>

double GetTimeSince1970()
{
    struct timeval timeCur;
    gettimeofday(&timeCur, NULL);
    return (timeCur.tv_sec * 1000.0 + timeCur.tv_usec / 1000.0);
}

#else

double GetTimeSince1970()
{
	eInt64  intervals;
    FILETIME  ft;
    GetSystemTimeAsFileTime(&ft);

    /*
     * A file time is a 64-bit value that represents the number
     * of 100-nanosecond intervals that have elapsed since
     * January 1, 1601 12:00 A.M. UTC.
     *
     * Between January 1, 1970 (Epoch) and January 1, 1601 there were
     * 134744 days,
     * 11644473600 seconds or
     * 11644473600,000,000,0 100-nanosecond intervals.
     *
     * See also MSKB Q167296.
     */

    intervals = ((eInt64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

	struct timeval timeCur;
    timeCur.tv_sec = (long) (intervals / 10000000);
    timeCur.tv_usec = (long) ((intervals % 10000000) / 10);

	return (timeCur.tv_sec * 1000.0 + timeCur.tv_usec / 1000.0);
}

#endif

#pragma region 纳秒函数实现

//获得纳秒 嘀嗒数

#ifdef __APPLE__

int getUptimeInMilliseconds()
{
    const int64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t s_timebase_info;
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        mach_timebase_info(&s_timebase_info);
    });
    
    // mach_absolute_time() returns billionth of seconds,
    // so divide by one million to get milliseconds
    return (int)((mach_absolute_time() * s_timebase_info.numer) / (kOneMillion * s_timebase_info.denom));
}

uint64_t GetTickNanoSeconds()
{
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    //return mach_absolute_time();
}

#elif defined __linux__

#include <time.h>

uint64_t GetTickNanoSeconds()
{
    struct timespec curTime = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &curTime);
    
    const int64_t kOneMillion = 1000 * 1000 * 1000;
    return curTime.tv_sec * kOneMillion + curTime.tv_nsec;
}

#elif defined(WIN32)

uint64_t GetTickNanoSeconds()
{
    eInt64  intervals;
    FILETIME  ft;
    GetSystemTimeAsFileTime(&ft);

    /*
     * A file time is a 64-bit value that represents the number
     * of 100-nanosecond intervals that have elapsed since
     * January 1, 1601 12:00 A.M. UTC.
     *
     * Between January 1, 1970 (Epoch) and January 1, 1601 there were
     * 134744 days,
     * 11644473600 seconds or
     * 11644473600,000,000,0 100-nanosecond intervals.
     *
     * See also MSKB Q167296.
     */

    intervals = ((eInt64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

	struct timeval timeCur;
    timeCur.tv_sec = (long) (intervals / 10000000);
    timeCur.tv_usec = (long) ((intervals % 10000000) / 10);

	const uint64_t kOneMillion = 1000 * 1000 * 1000;

	return (timeCur.tv_sec * kOneMillion + timeCur.tv_usec * 1000.0);
}

#endif

#pragma endregion 纳秒函数实现


NS_BASELIB_END
