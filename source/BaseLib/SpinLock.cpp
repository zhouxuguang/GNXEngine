#include "SpinLock.h"

NS_BASELIB_BEGIN

#ifdef _WIN32

//WINDOWS实现

#include <intrin.h>

SpinLock::SpinLock(void)
{
	_InterlockedAnd((volatile LONG*)&m_SpinLock,0);
}

SpinLock::~SpinLock(void)
{
	_InterlockedAnd((volatile LONG*)&m_SpinLock,0);
}

void SpinLock::Lock()
{
    for (unsigned int k = 0; !TryLock(); ++k)
    {
        Sleep(0);
    }
	
}

void SpinLock::UnLock()
{
    _ReadWriteBarrier();
	_InterlockedAnd((volatile LONG*)&m_SpinLock,0);
}

bool SpinLock::TryLock()
{
    long r = InterlockedExchange((volatile LONG *)&m_SpinLock, 1);
    _ReadWriteBarrier();                    // 内存栅栏
    return r == 0;
}

//////////////////////////////////////////////////////////////////////

#elif defined(__APPLE__)

//苹果系统实现

SpinLock::SpinLock(void)
{
    m_SpinLock = OS_SPINLOCK_INIT;
}

SpinLock::~SpinLock(void)
{
    assert(1);
    //不需要实现
}

void SpinLock::Lock()
{
    OSSpinLockLock(&m_SpinLock);
    
#if __has_feature(objc_arc)
    // ARC

#else
    // MRC
#endif
    
}

bool SpinLock::TryLock()
{
    return OSSpinLockTry(&m_SpinLock);
}

void SpinLock::UnLock()
{
    OSSpinLockUnlock(&m_SpinLock);
}

//////////////////////////////////////////////////////////////////////

#elif defined(__linux__)

SpinLock::SpinLock(void)
{
    __sync_lock_release(&m_SpinLock);
}

SpinLock::~SpinLock(void)
{
    __sync_lock_release(&m_SpinLock);
}

void SpinLock::Lock()
{
    while (!TryLock())
    {
        
    }
    
}

bool SpinLock::TryLock()
{
    return !__sync_lock_test_and_set(&m_SpinLock,1);
}

void SpinLock::UnLock()
{
    __sync_lock_release(&m_SpinLock);
}

//

//其他平台实现

#endif



NS_BASELIB_END
