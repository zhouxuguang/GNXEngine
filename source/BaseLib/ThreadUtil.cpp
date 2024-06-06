//
//  ThreadUtil.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/12/27.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "ThreadUtil.h"
#include "Thread.h"

#ifdef __ANDROID__
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef WIN32

//windows线程设置和获得函数

static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadName( DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
    
    __try
    {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

static char* GetThreadName(void)
{
    char* pszName = NULL;
    __asm
    {
        mov eax, fs:[0x18]    //    Locate the caller's TIB
        mov eax, [eax+0x14]    //    Read the pvArbitary field in the TIB
        mov [pszName], eax    //    pszName = pTIB->pvArbitary
    }
    return pszName ? pszName : "Win32 Thread";
}

#include <intrin.h>

namespace windows_pthread
{
    //windows pthread_once实现
    
    typedef volatile long pthread_once_t;
    
    static int pthread_once(pthread_once_t *o, void (*func)(void))
    {
        int state = *o;
        
        _ReadWriteBarrier();
        
        while (state != 1)
        {
            if (!state)
            {
                if (!InterlockedCompareExchange(o, 2, 0))
                {
                    /* Success */
                    func();
                    
                    /* Mark as done */
                    *o = 1;
                    
                    return 0;
                }
            }
            
            YieldProcessor();
            
            _ReadWriteBarrier();
            
            state = *o;
        }
        
        /* Done */
        return 0;
    }
}

#endif


NS_BASELIB_BEGIN

extern thread_handle_t GetCurrentThreadHangle();

thread_handle_t ThreadUtil::GetCurrentID()
{
    return GetCurrentThreadHangle();
}

#ifdef _WIN32

void ThreadUtil::Sleep(long long nMiliSeconds)
{
    ::Sleep(nMiliSeconds);
}

#else

void ThreadUtil::Sleep(long long nMiliSeconds)
{
    usleep((unsigned int)nMiliSeconds*1000);
}

#endif

bool ThreadUtil::ThreadYield()
{
#ifdef __APPLE__
    pthread_yield_np();
    return true;
#elif __linux__
    sched_yield();
    return true;
#elif _WIN32
    ::Sleep(0);
    return true;
#endif
}

//检测是否是主线程
#ifdef WIN32

#include <tlhelp32.h>

static DWORD GetMainThreadId(DWORD processId = 0)
{
    if (processId == 0)
        processId = GetCurrentProcessId();
    
    DWORD threadId = 0;
    THREADENTRY32 te32 = { sizeof(te32) };
    HANDLE threadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Thread32First(threadSnap, &te32))
    {
        do
        {
            if( processId == te32.th32OwnerProcessID )
            {
                threadId = te32.th32ThreadID;
                break;
            }
        } while(Thread32Next(threadSnap, &te32));
    }
    return threadId;
}

#endif

bool ThreadUtil::IsMainThread()
{
#if defined __APPLE__
    return pthread_main_np() != 0;
#elif defined __linux__
    return gettid() == getpid();
#else
    return GetCurrentThreadId() == GetMainThreadId();
#endif
    
}

bool ThreadUtil::ThreadOnceCall(thread_once_t nInitValue, OnceInitProc pInitFunc)
{
#ifdef WIN32
    using namespace windows_pthread;
#endif
    return 0 == pthread_once(&nInitValue,pInitFunc);
}

void ThreadUtil::SetName(const char *pszName)
{
#ifdef __ANDROID__
    //pthread_setname_np(m_hThread, pszName);
    prctl(PR_SET_NAME,pszName);
#elif __APPLE__
    pthread_setname_np(pszName);
#elif WIN32
    SetThreadName(-1, pszName);
#endif
}

void ThreadUtil::GetName(char *pszName, int len)
{
#ifdef __APPLE__
    pthread_getname_np(pthread_self(), pszName, len);
#elif __ANDROID__
    prctl(PR_GET_NAME,pszName);
#elif WIN32
    const char *pszNameWin = GetThreadName();
    memset(pszName, 0, len);
    memcpy(pszName, pszNameWin, strlen(pszNameWin));
#endif
}

NS_BASELIB_END
