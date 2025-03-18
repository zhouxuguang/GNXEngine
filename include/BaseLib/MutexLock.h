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

	void Lock() const;

	bool TryLock() const;
    
    bool TryLock(unsigned long msecs);

	void UnLock() const;

private:
	MutexHandle m_Lock;
};


//���࣬ʹ�÷�����AutoLock( &Mutex );
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
