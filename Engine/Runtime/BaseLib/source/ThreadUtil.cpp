//
//  ThreadUtil.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/12/27.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "ThreadUtil.h"
#include "Thread.h"

#if GNX_OS_ANDROID
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#elif GNX_OS_LINUX
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

static void SetThreadName(DWORD dwThreadID, const char* threadName)
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
    ::Sleep((DWORD)nMiliSeconds);
}

#else

void ThreadUtil::Sleep(long long nMiliSeconds)
{
    usleep((unsigned int)nMiliSeconds * 1000);
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
    return 0 == pthread_once(&nInitValue, pInitFunc);
}

void ThreadUtil::SetName(const char *pszName)
{
#ifdef __ANDROID__
    prctl(PR_SET_NAME,pszName);
#elif __APPLE__
    pthread_setname_np(pszName);
#elif WIN32
    SetThreadName(-1, pszName);
#endif
}

void ThreadUtil::GetName(char *pszName, int len)
{
    if (pszName == nullptr || len <= 0)
    {
        return;
    }

#ifdef __APPLE__
    pthread_getname_np(pthread_self(), pszName, len);
#elif __ANDROID__
    prctl(PR_GET_NAME, pszName);
#elif WIN32
    // Windows 10 1607+ 提供 GetThreadDescription 读取真实线程名。
    // 为避免依赖 _WIN32_WINNT >= 0x0A00 的编译期宏，这里运行时动态加载，
    // 加载失败（老系统）时回退到默认名称。
    memset(pszName, 0, len);

    typedef HRESULT(WINAPI* PFN_GetThreadDescription)(HANDLE, PWSTR*);
    static PFN_GetThreadDescription pfnGetThreadDescription =
        (PFN_GetThreadDescription)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadDescription");

    bool bGotName = false;
    if (pfnGetThreadDescription != nullptr)
    {
        PWSTR pszDesc = nullptr;
        if (SUCCEEDED(pfnGetThreadDescription(GetCurrentThread(), &pszDesc)) && pszDesc != nullptr)
        {
            int nLen = WideCharToMultiByte(CP_UTF8, 0, pszDesc, -1, nullptr, 0, nullptr, nullptr);
            if (nLen > 1 && nLen <= len)
            {
                WideCharToMultiByte(CP_UTF8, 0, pszDesc, -1, pszName, len, nullptr, nullptr);
                bGotName = true;
            }
            LocalFree(pszDesc);
        }
    }

    if (!bGotName)
    {
        strncpy(pszName, "Win32 Thread", len - 1);
        pszName[len - 1] = 0;
    }
#endif
}

NS_BASELIB_END
