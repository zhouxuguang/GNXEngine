#include "TimeCost.h"

NS_BASELIB_BEGIN

#ifdef WIN32

#include <sys/timeb.h>

static uint64_t GetTimeMillis()
{
	//return GetTickCount64();
    LARGE_INTEGER nFreq;
    if(QueryPerformanceFrequency(&nFreq))
    {
        struct __timeb64 tb;
        _ftime64(&tb);
        return (tb.time*1000ll + tb.millitm) * 1000;
    }
    
    else
    {
		LARGE_INTEGER nCurTime;
        QueryPerformanceCounter(&nCurTime);
        return (double)(nCurTime.QuadPart)/(double)nFreq.QuadPart;
    }
}

#elif defined __ANDROID__ || __linux__

#include <sys/time.h>


//mico second
static uint64_t GetTimeMillis()
{
    //���ȶ�������
	//struct timeb64 tb;
	//ftime(&tb);

	//return tb.time*1000ll + tb.millitm;
    
//    timeval tv = {0};
//    gettimeofday(&tv,NULL);
//
//    return tv.tv_sec * 1000000 + tv.tv_usec;
    
    struct timespec tmSpec;
    clock_gettime(CLOCK_MONOTONIC,&tmSpec);

    return tmSpec.tv_sec * 1000000000 + tmSpec.tv_nsec;
}

#elif defined __APPLE__

#include <mach/mach_time.h>

static uint64_t GetTimeMillis()
{
    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);
    
    uint64_t timeNum = mach_absolute_time();
    timeNum *= (timebase_info.numer / timebase_info.denom);
    return timeNum;
}

#endif

TimeCost::TimeCost(void)
{
	m_nStart = GetTimeMillis();
	m_nEnd = m_nStart;
}

TimeCost::~TimeCost(void)
{
}

void TimeCost::Begin()
{
	m_nStart = GetTimeMillis();
}

void TimeCost::End()
{
	m_nEnd = GetTimeMillis();
}

uint64_t TimeCost::GetCostTime() const
{
	return (m_nEnd - m_nStart) * 0.001;
}

uint64_t TimeCost::GetCostTimeNano() const
{
    return m_nEnd - m_nStart;
}

NS_BASELIB_END
