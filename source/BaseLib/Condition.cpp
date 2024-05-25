//
//  Condition.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/8/2.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "Condition.h"
#include "MutexLock.h"

NS_BASELIB_BEGIN


#if defined __linux__ || defined __ANDROID__ || ( defined(__MACH__) && defined(__APPLE__) )

struct LinuxMutex
{
    pthread_mutex_t mutex_t;
    pthread_mutexattr_t mutex_attr_t;
};

#include <sys/time.h>
Condition::Condition(MutexLock* mutexLock) : lock(mutexLock)
{
    pthread_cond_init(&condId,NULL);
}

Condition::~Condition()
{
    pthread_cond_destroy(&condId);
}

void Condition::Wait()
{
    LinuxMutex* pMutexData = (LinuxMutex*)lock->m_Lock;
    if (!pMutexData)
    {
        return;
    }
    pthread_cond_wait(&condId,&pMutexData->mutex_t);
}

void Condition::Wait(long long mills)
{
    struct timespec timeToWait;
    struct timeval now;
    
    gettimeofday(&now,NULL);
    
    long seconds = mills/1000;
    long nanoseconds = (mills - seconds * 1000) * 1000000;
    timeToWait.tv_sec = now.tv_sec + seconds;
    timeToWait.tv_nsec = now.tv_usec*1000 + nanoseconds;
    
    if (timeToWait.tv_nsec >= 1000000000)
    {
        timeToWait.tv_nsec -= 1000000000;
        timeToWait.tv_sec++;
    }
    
    LinuxMutex* pMutexData = (LinuxMutex*)lock->m_Lock;
    if (!pMutexData)
    {
        return;
    }
    pthread_cond_timedwait(&condId,&pMutexData->mutex_t,&timeToWait);
}

void Condition::Notify()
{
    pthread_cond_signal(&condId);
}

void Condition::NotifyAll()
{
    pthread_cond_broadcast(&condId);
}

#elif WIN32

struct WindowsMutex
{
	CRITICAL_SECTION lock_t;
};


#if _WIN32_WINNT >= 0x0600

//windows下只有Vista之后的版本才能使用，在这个之下的版本需要用mutex模拟

Condition::Condition(MutexLock* lock)
{
    this->lock = lock;
    InitializeConditionVariable(&condId);
}

Condition::~Condition()
{
    //windows下不需要销毁
}

void Condition::Wait()
{
	WindowsMutex* pMutexData = (WindowsMutex*)lock->m_Lock;
    SleepConditionVariableCS(&condId,&pMutexData->lock_t,INFINITE);
}

void Condition::Wait(long long mills)
{
	WindowsMutex* pMutexData = (WindowsMutex*)lock->m_Lock;
    SleepConditionVariableCS(&condId,&pMutexData->lock_t,(DWORD)mills);
}

void Condition::Notify()
{
    WakeConditionVariable(&condId);
}

void Condition::NotifyAll()
{
    WakeAllConditionVariable(&condId);
}

#else
	//vista之下的版本

static void WINAPI init_condvar_using_event( condition_variable_with_event* cv_event )
{
	if (NULL == cv_event)
	{
		return;
	}
	
	// TODO: For Metro port, we can always use the API for condition variables, without dynamic_link etc.
	cv_event->event = CreateEventEx(NULL, NULL, 0x1, EVENT_ALL_ACCESS );
	InitializeCriticalSectionEx( &cv_event->mutex, 4000, 0 );
	cv_event->n_waiters = 0;
	cv_event->release_count = 0;
	cv_event->epoch = 0;
}

static BOOL WINAPI sleep_condition_variable_cs_using_event( condition_variable_with_event* cv_event, 
	LPCRITICAL_SECTION cs, DWORD dwMilliseconds )
{
	EnterCriticalSection( &cv_event->mutex );
	++cv_event->n_waiters;
	unsigned my_generation = cv_event->epoch;
	LeaveCriticalSection( &cv_event->mutex );
	LeaveCriticalSection( cs );
	while (1)	
	{
		// should come here at least once
		DWORD rc = WaitForSingleObjectEx( cv_event->event, dwMilliseconds, FALSE );
		EnterCriticalSection( &cv_event->mutex );
		if( rc!=WAIT_OBJECT_0 ) 
		{
			--cv_event->n_waiters;
			LeaveCriticalSection( &cv_event->mutex );
			if( rc==WAIT_TIMEOUT ) 
			{
				SetLastError( WAIT_TIMEOUT );
				EnterCriticalSection( cs );
			}
			return false;
		}
		//__TBB_ASSERT( rc==WAIT_OBJECT_0, NULL );
		if( cv_event->release_count>0 && cv_event->epoch!=my_generation )
			break;
		LeaveCriticalSection( &cv_event->mutex );
	}

	// still in the critical section
	--cv_event->n_waiters;
	int count = --cv_event->release_count;
	LeaveCriticalSection( &cv_event->mutex );

	if( 0 == count ) 
	{
		//__TBB_ASSERT( cv_event->event, "Premature destruction of condition variable?" );
		ResetEvent( cv_event->event );
	}
	EnterCriticalSection( cs );
	return true;
}

static void WINAPI wake_condition_variable_using_event( condition_variable_with_event* cv_event )
{
	EnterCriticalSection( &cv_event->mutex );
	if( cv_event->n_waiters > cv_event->release_count ) 
	{
		SetEvent( cv_event->event ); // Signal the manual-reset event.
		++cv_event->release_count;
		++cv_event->epoch;
	}
	LeaveCriticalSection( &cv_event->mutex );
}

static void WINAPI wake_all_condition_variable_using_event( condition_variable_with_event* cv_event )
{
	EnterCriticalSection( &cv_event->mutex );
	if( cv_event->n_waiters>0 ) 
	{
		SetEvent( cv_event->event );
		cv_event->release_count = cv_event->n_waiters;
		++cv_event->epoch;
	}
	LeaveCriticalSection( &cv_event->mutex );
}

static void WINAPI destroy_condvar_using_event( condition_variable_with_event* cv_event )
{
	HANDLE my_event = cv_event->event;
	EnterCriticalSection( &cv_event->mutex );
	// NULL is an invalid HANDLE value
	cv_event->event = NULL;
	if( cv_event->n_waiters>0 ) 
	{
		LeaveCriticalSection( &cv_event->mutex );

		//直到等待者的个数等于0
		while( cv_event->n_waiters == 0 ){}
		// make sure the last thread completes its access to cv
		EnterCriticalSection( &cv_event->mutex );
	}
	LeaveCriticalSection( &cv_event->mutex );
	CloseHandle( my_event );
}

Condition::Condition(MutexLock* lock)
{
	this->lock = lock;
	init_condvar_using_event(&this->condId);
}

Condition::~Condition()
{
	this->lock = NULL;
	destroy_condvar_using_event(&this->condId);
}

void Condition::Wait()
{
	WindowsMutex* pMutexData = (WindowsMutex*)lock->m_Lock;
	sleep_condition_variable_cs_using_event(&this->condId,&pMutexData->lock_t,INFINITE);
}

void Condition::Wait(long long time)
{
	WindowsMutex* pMutexData = (WindowsMutex*)lock->m_Lock;
	sleep_condition_variable_cs_using_event(&this->condId,&pMutexData->lock_t,(DWORD)time);
}

void Condition::Notify()
{
	wake_condition_variable_using_event(&this->condId);
}

void Condition::NotifyAll()
{
	wake_all_condition_variable_using_event(&this->condId);
}

#endif

#endif


NS_BASELIB_END

	/*Window各个版本对应的宏值

	Minimum system required

	Macros to define

	Win8

	_WIN32_WINNT>=0x0602 
	WINVER>=0x0602

	Win7

	_WIN32_WINNT>=0x0601 
	WINVER>=0x0601

	Windows Server 2008 R2

	_WIN32_WINNT>=0x0601 
	WINVER>=0x0601

	Windows Server 2008

	_WIN32_WINNT>=0x0600 
	WINVER>=0x0600

	Windows Vista

	_WIN32_WINNT>=0x0600 
	WINVER>=0x0600

	Windows Server 2003

	_WIN32_WINNT>=0x0502 
	WINVER>=0x0502

	Windows XP

	_WIN32_WINNT>=0x0501 
	WINVER>=0x0501

	Windows 2000

	_WIN32_WINNT>=0x0500 
	WINVER>=0x0500

	Windows NT 4.0

	_WIN32_WINNT>=0x0400 
	WINVER>=0x0400

	Windows Me

	_WIN32_WINDOWS=0x0500 
	WINVER>=0x0500

	Windows 98

	_WIN32_WINDOWS>=0x0410 
	WINVER>=0x0410

	Windows 95

	_WIN32_WINDOWS>=0x0400 
	WINVER>=0x0400*/
