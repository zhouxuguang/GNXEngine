#ifndef MUTEXLOCK_INCLUDE_04587955
#define MUTEXLOCK_INCLUDE_04587955

#include "PreCompile.h"

NS_BASELIB_BEGIN

/**
 * SimpleMutex 简单的互斥锁，有futex实现和std::mutex实现，主要用于不用和条件变量交互的场景
 */
#if USE_FUTEX

class BASELIB_API MutexOnFutex
{
public:
	void lock();
	void unlock();
    
    MutexOnFutex(const MutexOnFutex&) = delete;
    MutexOnFutex& operator=(const MutexOnFutex&) = delete;
private:
	void futexWait();
	void futexWake();

#if OS_MACOS
    os_unfair_lock mLock = OS_UNFAIR_LOCK_INIT;
#else
    std::atomic_uint32_t mState = 0;
#endif
};

using SimpleMutex = MutexOnFutex;

#else

class BASELIB_API MutexOnStd
{
public:
	void lock();
	void unlock();

    MutexOnStd(const MutexOnStd&) = delete;
    MutexOnStd& operator=(const MutexOnStd&) = delete;
private:
	std::mutex mutex;
};

using SimpleMutex = MutexOnStd;

#endif

class BASELIB_API MutexLock
{
public:
    friend class Condition;
    
	MutexLock(void);

	~MutexLock(void);

	void Lock();

	bool TryLock();
    
    bool TryLock(unsigned long msecs);

	void UnLock();

    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;
private:
#if OS_WINDOWS
    CRITICAL_SECTION mLock;
#else
    pthread_mutex_t mLock;
    pthread_mutexattr_t mLockAttr;
#endif
};


//自动锁(RAII) AutoLock(Mutex);
class BASELIB_API AutoLock
{
public:    
    explicit AutoLock(MutexLock& Mutex);
    
    ~AutoLock();

    AutoLock(const AutoLock&) = delete;
    AutoLock& operator=(const AutoLock&) = delete;
private:
    MutexLock& mMutex;
};

NS_BASELIB_END

#endif
