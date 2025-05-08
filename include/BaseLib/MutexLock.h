#ifndef MUTEXLOCK_INCLUDE_04587955
#define MUTEXLOCK_INCLUDE_04587955

#include "PreCompile.h"

NS_BASELIB_BEGIN

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

private:
#if OS_WINDOWS
    CRITICAL_SECTION mLock;
#endif
	MutexHandle m_Lock;
};


//自动锁(RAII) AutoLock( Mutex );
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
