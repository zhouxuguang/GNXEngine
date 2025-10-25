#ifndef BASELIB_SPINLOCK_INCLUDE_H_DKF87G
#define BASELIB_SPINLOCK_INCLUDE_H_DKF87G

//自旋锁的接口

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API SpinLock
{
public:
	SpinLock(void);

	~SpinLock(void);

	void Lock();

	bool TryLock();

	void UnLock();

private:
	spin_lock_t m_SpinLock;

	SpinLock(const SpinLock&);
	SpinLock& operator= (const SpinLock&);
};

class BASELIB_API SpinLockGuard
{
public:
    SpinLockGuard(SpinLock &spinLock) : mSpinLock(spinLock)
    {
        mSpinLock.Lock();
    }
    
    ~SpinLockGuard()
    {
        mSpinLock.UnLock();
    }
    
private:
    SpinLock &mSpinLock;
    
    SpinLockGuard(const SpinLockGuard&);
    SpinLockGuard& operator= (const SpinLockGuard&);
};

NS_BASELIB_END

#endif
