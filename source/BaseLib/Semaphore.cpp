//
//  Semaphore.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/6/18.
//  Copyright 漏 2016骞�zhouxuguang. All rights reserved.
//

#include "Semaphore.h"

NS_BASELIB_BEGIN

#ifdef WIN32

Semaphore::Semaphore(int initValue)
{
	m_sem = ::CreateSemaphoreA(
		NULL, // security attributes
		initValue, // initial count
		0x7fffffff, // maximum value
		NULL); // name
	assert(NULL != m_sem);
}

Semaphore::~Semaphore()
{
	BOOL rs = ::CloseHandle(m_sem);
	assert(rs);
}

void Semaphore::wait()
{
	DWORD rs = ::WaitForSingleObject(m_sem, INFINITE);
	assert(WAIT_OBJECT_0 == rs);
}

void Semaphore::post()
{
	BOOL rs = ::ReleaseSemaphore(m_sem, 1, NULL);
	assert(rs);
}

bool Semaphore::tryWait()
{
	return WAIT_OBJECT_0 == ::WaitForSingleObject(m_sem, 0);
}

bool Semaphore::timedWait(unsigned int s, unsigned int ms)
{
	DWORD dwMilliseconds = s * 1000 + ms;
	return WAIT_OBJECT_0 == ::WaitForSingleObject(m_sem, dwMilliseconds);
}


#elif defined(__linux__) || defined(__ANDROID__)


Semaphore::Semaphore(int initValue)
{
    ::sem_init(&m_sem,0,initValue);
}

Semaphore::~Semaphore()
{
    ::sem_destroy(&m_sem);
}

void Semaphore::wait()
{
    ::sem_wait(&m_sem);
}

void Semaphore::post()
{
    ::sem_post(&m_sem);
}

bool Semaphore::tryWait()
{
    return 0 == ::sem_trywait(&m_sem);
}

bool Semaphore::timedWait(unsigned int s, unsigned int ms)
{
    struct timespec abstime;
    abstime.tv_sec += s;
    abstime.tv_nsec += ((long)ms) * 1000 * 1000;
    return 0 == ::sem_timedwait(&m_sem, &abstime);
}

#elif ( defined(__MACH__) && defined(__APPLE__) )        //�规���OS
Semaphore::Semaphore(int initValue)
{
    m_sem = dispatch_semaphore_create(initValue);
}

Semaphore::~Semaphore()
{
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 60000
    dispatch_release(m_sem);
#endif
}

void Semaphore::wait()
{
    dispatch_semaphore_wait(m_sem,DISPATCH_TIME_FOREVER);
}

void Semaphore::post()
{
    dispatch_semaphore_signal(m_sem);
}

bool Semaphore::tryWait()
{
    return dispatch_semaphore_wait(m_sem,DISPATCH_TIME_NOW);
}

bool Semaphore::timedWait(unsigned int s, unsigned int ms)
{
    struct timespec abstime;
    abstime.tv_sec += s;
    abstime.tv_nsec += ((long)ms) * 1000 * 1000;
    
    return dispatch_semaphore_wait(m_sem,s * 1000 + ms);
}


#endif



NS_BASELIB_END
