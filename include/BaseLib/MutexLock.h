#ifndef MUTEXLOCK_INCLUDE_04587955
#define MUTEXLOCK_INCLUDE_04587955

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API MutexLock
{
public:
    friend class Condition;
    
	MutexLock(void);
    
    explicit MutexLock(const char* name);
    
    explicit MutexLock(int type, const char* name = NULL);

	~MutexLock(void);

	//加锁
	void Lock() const;

	//尝试加锁
	bool TryLock() const;
    
    //尝试加锁，msecs时间
    bool TryLock(unsigned long msecs);

	//解锁
	void UnLock() const;

private:
	MutexHandle m_Lock;
};


//锁类，使用方法是AutoLock( &Mutex );
class BASELIB_API AutoLock
{
public:
    explicit AutoLock( const MutexLock* Mutex );
    
    explicit AutoLock( const MutexLock& Mutex );
    
    ~AutoLock();
private:
    const MutexLock* mMutex;
};

NS_BASELIB_END

#endif
