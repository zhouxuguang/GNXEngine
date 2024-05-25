#include "MutexLock.h"

NS_BASELIB_BEGIN

#if defined OS_WINDOWS 
#include <windows.h>

struct WindowsMutex
{
	CRITICAL_SECTION lock_t;
};

MutexLock::MutexLock(void)
{
	WindowsMutex* pMutexData = new(std::nothrow) WindowsMutex();

    BOOL success = InitializeCriticalSectionAndSpinCount(&pMutexData->lock_t,4096);
    assert(TRUE == success);

	m_Lock = pMutexData;
}

MutexLock::~MutexLock(void)
{
	WindowsMutex* pMutexData = (WindowsMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
	DeleteCriticalSection(&pMutexData->lock_t);

	delete pMutexData;
	m_Lock = NULL;
}

void MutexLock::Lock() const
{
	WindowsMutex* pMutexData = (WindowsMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
	EnterCriticalSection(&pMutexData->lock_t);
}

bool MutexLock::TryLock() const
{
	WindowsMutex* pMutexData = (WindowsMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return false;
    }
	return TryEnterCriticalSection(&pMutexData->lock_t);
}

bool MutexLock::TryLock(unsigned long msecs)
{
	WindowsMutex* pMutexData = (WindowsMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return false;
    }

	bool bRet = true;
	DWORD dTimeStart = GetTickCount();
	while (!TryEnterCriticalSection(&pMutexData->lock_t))
	{
		DWORD dTimeEnd = GetTickCount();
		if (dTimeEnd - dTimeStart >= msecs)
		{
			bRet = false;
		}
	}

	return bRet;
}

void MutexLock::UnLock() const
{
	WindowsMutex* pMutexData = (WindowsMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
	LeaveCriticalSection((LPCRITICAL_SECTION)&pMutexData->lock_t);
}

#else
#include <pthread.h>

struct LinuxMutex
{
    pthread_mutex_t mutex_t;
    pthread_mutexattr_t mutex_attr_t;
};

extern unsigned long GetTickCount(void);

//NDK pthread_mutex_timedlock代替pthread_mutex_lock_timeout_np

MutexLock::MutexLock(void)
{
    LinuxMutex* pMutexData = new(std::nothrow) LinuxMutex();
    int result = pthread_mutexattr_init(&pMutexData->mutex_attr_t);
    // successful = 0
    // errors = ENOMEM
    
    result = pthread_mutexattr_settype(&pMutexData->mutex_attr_t,PTHREAD_MUTEX_RECURSIVE);
    // successful = 0
    
    result = pthread_mutex_init(&pMutexData->mutex_t,&pMutexData->mutex_attr_t);
    
    m_Lock = pMutexData;
}

MutexLock::~MutexLock(void)
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
    int result = pthread_mutex_destroy(&pMutexData->mutex_t);
    // successful = 0
    // errors = EINVAL
    
    result = pthread_mutexattr_destroy(&pMutexData->mutex_attr_t);
    // successful = 0
    // errors = EBUSY, EINVAL
    
    delete pMutexData;
    m_Lock = NULL;
}

void MutexLock::Lock() const
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
	pthread_mutex_lock(&pMutexData->mutex_t);
}

bool MutexLock::TryLock() const
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return false;
    }
    return 0 == pthread_mutex_trylock(&pMutexData->mutex_t);
}

#ifdef __ANDROID__
bool MutexLock::TryLock(unsigned long msecs)
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return false;
    }
#ifndef __LP64__
    int ret = pthread_mutex_lock_timeout_np(&pMutexData->mutex_t,(unsigned)msecs);
    return ret;
    
#else
    
    //ANDROID标准实现方法  //pthread_mutex_timedlock  armeabi-v7a不支持pthread_mutex_timedlock
    
    struct timespec timeToWait;
    struct timeval now;
    
    gettimeofday(&now,NULL);
    
    long seconds = msecs/1000;
    long nanoseconds = (msecs - seconds * 1000) * 1000000;
    timeToWait.tv_sec = now.tv_sec + seconds;
    timeToWait.tv_nsec = now.tv_usec*1000 + nanoseconds;
    
    if (timeToWait.tv_nsec >= 1000000000)
    {
        timeToWait.tv_nsec -= 1000000000;
        timeToWait.tv_sec++;
    }
    
    return pthread_mutex_timedlock(&pMutexData->mutex_t, &timeToWait) == 0;
    
#endif

}

#else
#include <sys/time.h>
#include <errno.h>

int pthread_mutex_timedlock (pthread_mutex_t *mutex, const struct timespec *timeout)
{
    struct timeval timenow;
    struct timespec sleepytime;
    int retcode;
    
    /* This is just to avoid a completely busy wait */
    sleepytime.tv_sec = 0;
    sleepytime.tv_nsec = 10000000; /* 10ms */
    
    while ((retcode = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        gettimeofday(&timenow, NULL);
        
        if (timenow.tv_sec >= timeout->tv_sec && (timenow.tv_usec * 1000) >= timeout->tv_nsec)
        {
            return ETIMEDOUT;
        }
        
        nanosleep(&sleepytime, NULL);
    }
    
    return retcode;
}

bool MutexLock::TryLock(unsigned long mills)
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return false;
    }
#if defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS - 200112L) >= 0L
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
    return pthread_mutex_timedlock(&pMutexData->mutex_t,&timeToWait) == 0;

#else
    unsigned long nStartTime = GetTickCount();
    while ( !pthread_mutex_trylock(&pMutexData->mutex_t) )
    {
        unsigned long nStopTime = GetTickCount();
        if (nStopTime - nStartTime >= mills)
        {
            return false;
        }
    }
    
    return true;
#endif
}
#endif

void MutexLock::UnLock() const
{
    LinuxMutex* pMutexData = (LinuxMutex*)m_Lock;
    if (NULL == pMutexData)
    {
        return;
    }
	pthread_mutex_unlock(&pMutexData->mutex_t);
}

#endif

AutoLock::AutoLock( const MutexLock* Mutex ) : mMutex(Mutex)
{
    mMutex->Lock();
}

AutoLock::AutoLock( const MutexLock& Mutex ): mMutex(&Mutex)
{
    mMutex->Lock();
}

AutoLock::~AutoLock()
{
    mMutex->UnLock();
}


NS_BASELIB_END
