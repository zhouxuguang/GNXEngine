#include "Timer.h"

NS_BASELIB_BEGIN

//
void Timer::DestroyTimer(Timer* pTimer)
{
	delete pTimer;
}

Timer::Timer(TimerOption *pOption)
{
	m_Option = pOption;
}

Timer::~Timer(void)
{
	UnInit();
}

//end

#ifdef WIN32

#include <Windows.h>

struct WindowsTimeFuncST
{
	TimerProc pTimerFun;
	void* pParameter;
	~WindowsTimeFuncST()
	{
		printf("WindowsTimeFuncST destroy\n");
	}
};

struct TimerOption
{
	HANDLE hTimerQueue;
	HANDLE hTimerId;
	WindowsTimeFuncST *pFuncPara;
};

VOID NTAPI  WindowsTimeFunc (PVOID pPara, BOOLEAN flag)
{
	WindowsTimeFuncST *stTimeFunc = (WindowsTimeFuncST *)pPara;
	TimerProc pTimerFun = stTimeFunc->pTimerFun;
	void* pParameter = stTimeFunc->pParameter;

	if (pTimerFun != NULL)
	{
		pTimerFun(pParameter,flag);
	}
	
}

Timer* Timer::CreateTimer(int64_t nStartLater,int64_t nInterval,TimerProc pFun,void* pParameter)
{
	HANDLE hTimer = INVALID_HANDLE_VALUE;
	HANDLE hQueue = CreateTimerQueue();
	if (hQueue == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	WindowsTimeFuncST *stTimeFunc = new(std::nothrow) WindowsTimeFuncST;
	stTimeFunc->pTimerFun = pFun;
	stTimeFunc->pParameter = pParameter;
	if (!CreateTimerQueueTimer(&hTimer,hQueue,WindowsTimeFunc,stTimeFunc,nStartLater,nInterval,0))
	{
		DeleteTimerQueue(hQueue);
		return NULL;
	}

	TimerOption *pOption = new(std::nothrow) TimerOption;
	pOption->hTimerId = hTimer;
	pOption->hTimerQueue = hQueue;
	pOption->pFuncPara = stTimeFunc;
	
	return new Timer(pOption);
}

void Timer::UnInit()
{
	if (m_Option != NULL)
	{
		HANDLE hQueue = m_Option->hTimerQueue;
		HANDLE hTimerId = m_Option->hTimerId;
		DeleteTimerQueueTimer(hQueue, hTimerId, INVALID_HANDLE_VALUE);
		DeleteTimerQueue(hQueue);

		delete m_Option->pFuncPara;
		delete m_Option;
		m_Option = NULL;
	}
}

#elif defined __linux__ || defined __ANDROID__

//#if defined(__MACH__) && defined(__APPLE__)
//#include <sys/ioctl.h>
//#include <sys/mount.h>
//#include <sys/types.h>
//#include <sys/time.h>
//#include <ftw.h>
//#include <mach/mach_time.h>
//#include <inttypes.h>
//
//#define CLOCK_REALTIME ITIMER_REAL
//#define itimerspec itimerval
//typedef uint64_t timer_t;
//typedef double   timer_c;
//typedef clock_id_t clockid_t;
//
//#define sigval_t sigval
//
//
//static inline int timer_create (clockid_t __clock_id,
//                                 struct sigevent *__restrict __evp,
//                                 timer_t *__restrict timer)
//{
//    // set something, to initialize the variable, just in case
//    *timer = 0;
//    return 0;
//}
//
//static inline int timer_settime (timer_t timerid, int flags,
//                                  const struct itimerspec *__restrict timerspec,
//                                  struct itimerspec *__restrict ovalue)
//{
//    return setitimer(ITIMER_REAL, timerspec, ovalue);
//}
//
//static inline int timer_delete (timer_t timerid)
//{
//    struct itimerspec timespec;
//    timespec.it_interval.tv_sec=0;
//    timespec.it_interval.tv_usec=0;
//    timespec.it_value.tv_sec=0;
//    timespec.it_value.tv_usec=0;
//
//    return setitimer(ITIMER_REAL, &timespec, NULL);
//}
//
//static inline int timer_gettime (timer_t timerid, struct itimerspec *value)
//{
//    return getitimer(ITIMER_REAL, value);
//}
//#endif

struct FuncOption
{
	void* pArgs;		//参数
	uint8_t eArgs;		//参数
	TimerProc pFun;		//回调函数
};

struct TimerOption
{
	TimerOption()
	{
		timerid = 0;
		pOption = NULL;
	}

	~TimerOption()
	{
		timerid = 0;
		if(pOption != NULL)
		{
			delete pOption;
			pOption = NULL;
		}
	}
	timer_t timerid;
	FuncOption* pOption;
};

static void TimerFunction(sigval_t value)
{
	FuncOption* pOption = (FuncOption *)value.sival_ptr;
	void* pArgs = pOption->pArgs;
	uint8_t eArgs = pOption->eArgs;
	TimerProc pFun = pOption->pFun;

	//delete pOption;
	pFun(pArgs,eArgs);
}

Timer* Timer::CreateTimer(int64_t nStartLater,int64_t nInterval,TimerProc pFun,void* pParameter)
{
	timer_t timerid;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));       //清零初始化

	FuncOption* pOption = new FuncOption;
	pOption->eArgs = 0;
	pOption->pArgs = pParameter;
	pOption->pFun = pFun;

	//evp.sigev_value.sival_int = 111;
	evp.sigev_value.sival_ptr = pOption;
	evp.sigev_notify = SIGEV_THREAD;            //线程通知的方式，派驻新线程
	evp.sigev_notify_function = TimerFunction;       	//线程函数地址

	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		perror("fail to timer_create");
		return NULL;
	}

	struct itimerspec it;
	//设置间隔响应时间
	int nSecond = nInterval/1000;
	int nMilliSecond = nInterval%1000;
	it.it_interval.tv_sec = nSecond;
#if defined(__MACH__) && defined(__APPLE__)
	it.it_interval.tv_usec = nMilliSecond;
#else
    it.it_interval.tv_nsec = nMilliSecond;
#endif
	//设置第一次响应时间
	nSecond = nStartLater/1000;
	nMilliSecond = nStartLater%1000;
	it.it_value.tv_sec = nSecond;
#if defined(__MACH__) && defined(__APPLE__)
	it.it_value.tv_usec = nMilliSecond;
#else
    it.it_value.tv_nsec = nMilliSecond;
#endif

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		timer_delete(timerid);
		perror("fail to timer_settime");
		return NULL;
	}

	TimerOption* pTimeOpt = new TimerOption;
	pTimeOpt->timerid = timerid;
	pTimeOpt->pOption = pOption;

	return new Timer(pTimeOpt);
}

void Timer::UnInit()
{
	if(m_Option != NULL)
	{
		timer_t timerid = m_Option->timerid;
		timer_delete(timerid);
		delete m_Option;
		m_Option = NULL;
	}
}


#elif defined __IOS__ || ( defined(__MACH__) && defined(__APPLE__) )

#include <dispatch/dispatch.h>

struct TimerOption
{
	dispatch_queue_t queue;
	dispatch_source_t timer;
};

Timer* Timer::CreateTimer(int64_t nStartLater,int64_t nInterval,TimerProc pFun,void* pParameter)
{
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
	if (timer)
	{
        //gcd的时间单位是纳秒，注意时间转换
		dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, nInterval * NSEC_PER_MSEC), nInterval * NSEC_PER_MSEC, (1ull * NSEC_PER_MSEC) / 10);
        
        //设置执行的事件
        dispatch_source_set_event_handler(timer, ^{pFun(pParameter,false);});
		dispatch_resume(timer);

        TimerOption* pOption = new(std::nothrow) TimerOption;
		pOption->queue = queue;
		pOption->timer = timer;
		return new Timer(pOption);
	}

	return NULL;
}

void Timer::UnInit()
{
	if(m_Option != NULL)
	{
		dispatch_queue_t queue = m_Option->queue;
		dispatch_source_t timer = m_Option->timer;
		dispatch_source_cancel(timer);
        
        //6.0sdk之前
    #if __IPHONE_OS_VERSION_MIN_REQUIRED < 60000
		dispatch_release(timer);
    #endif
		timer = NULL;
		delete m_Option;
		m_Option = NULL;
	}
}

#endif


NS_BASELIB_END
