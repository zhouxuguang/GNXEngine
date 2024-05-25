#ifndef BASELIB_TIMER_INCLUDE_H_KG56GH
#define BASELIB_TIMER_INCLUDE_H_KG56GH

//跨平台定时器的封装

#include "PreCompile.h"

NS_BASELIB_BEGIN

struct TimerOption;

//定时器回调函数声明
typedef void (* TimerProc) (void*, uint8_t );

class BASELIB_API Timer
{
public:
    
    //注意时间单位是毫秒
	static Timer* CreateTimer(int64_t nStartLater,int64_t nInterval,TimerProc pFun,void* pParameter);

    //销毁定时器
	static void DestroyTimer(Timer* pTimer);

private:
	TimerOption *m_Option;

	Timer(TimerOption *pOption);

	~Timer(void);

	void UnInit();
};

NS_BASELIB_END

#endif
