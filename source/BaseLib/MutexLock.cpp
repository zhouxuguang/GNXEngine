#include "MutexLock.h"

NS_BASELIB_BEGIN

#if USE_FUTEX

// Note: the ordering of these values is important due to |unlock()|'s atomic decrement.
static constexpr uint32_t kUnlocked = 0;
static constexpr uint32_t kLocked = 1;
static constexpr uint32_t kBlocked = 2;

inline void MutexOnFutex::lock()
{
	uint32_t oldState = kUnlocked;
	const bool lockTaken = mState.compare_exchange_strong(oldState, kLocked);

	// In uncontended cases, the lock is acquired and there's nothing to do
	if (!lockTaken)
	{
		assert(oldState == kLocked || oldState == kBlocked);

		// If not already marked as such, signal that the mutex is contended.
		if (oldState != kBlocked)
		{
			oldState = mState.exchange(kBlocked, std::memory_order_acq_rel);
		}
		// Wait until the lock is acquired
		while (oldState != kUnlocked)
		{
			futexWait();
			oldState = mState.exchange(kBlocked, std::memory_order_acq_rel);
		}
	}
}

inline void MutexOnFutex::unlock()
{
	// Unlock the mutex
	const uint32_t oldState = mState.fetch_add(-1, std::memory_order_acq_rel);

	// If another thread is waiting on this mutex, wake it up
	if (oldState != kLocked)
	{
		mState.store(kUnlocked, std::memory_order_relaxed);
		futexWake();
	}
}

#else

inline void MutexOnStd::lock() { mutex.lock(); }

inline void MutexOnStd::unlock() { mutex.unlock(); }

#endif

#if defined OS_WINDOWS 
#include <windows.h>

#if USE_FUTEX
#if OS_WINDOWS

#pragma comment(lib, "Synchronization.lib")

void MutexOnFutex::futexWait()
{
	int value = kBlocked;
	WaitOnAddress(&mState, &value, sizeof(value), INFINITE);
}

void MutexOnFutex::futexWake()
{
	WakeByAddressSingle(&mState);
}

#else
//Linux实现
{
inline void SysFutex(void* addr, int op, int val, int val3)
{
	syscall(SYS_futex, addr, op, val, nullptr, nullptr, val3);
}
}

void MutexOnFutex::futexWait()
{
	SysFutex(&mState, FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG, kBlocked, FUTEX_BITSET_MATCH_ANY);
}
void MutexOnFutex::futexWake()
{
	SysFutex(&mState, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, kLocked, 0);
}
#endif
#endif

MutexLock::MutexLock(void)
{
    BOOL success = InitializeCriticalSectionAndSpinCount(&mLock, 4096);
    assert(TRUE == success);
}

MutexLock::~MutexLock(void)
{
	DeleteCriticalSection(&mLock);
}

void MutexLock::Lock()
{
	EnterCriticalSection(&mLock);
}

bool MutexLock::TryLock()
{
	return TryEnterCriticalSection(&mLock);
}

bool MutexLock::TryLock(unsigned long msecs)
{
	bool bRet = true;
	DWORD dTimeStart = GetTickCount();
	while (!TryEnterCriticalSection(&mLock))
	{
		DWORD dTimeEnd = GetTickCount();
		if (dTimeEnd - dTimeStart >= msecs)
		{
			bRet = false;
		}
	}

	return bRet;
}

void MutexLock::UnLock()
{
	LeaveCriticalSection(&mLock);
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

AutoLock::AutoLock(MutexLock& Mutex): mMutex(Mutex)
{
    mMutex.Lock();
}

AutoLock::~AutoLock()
{
    mMutex.UnLock();
}

NS_BASELIB_END
