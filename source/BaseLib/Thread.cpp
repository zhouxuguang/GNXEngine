
#include "Thread.h"

NS_BASELIB_BEGIN

//公共部分的函数

void Thread::SetStackSize(uint32_t unStackSize)
{
    m_unStackSize = unStackSize;
}

uint32_t Thread::GetStackSize() const
{
    return m_unStackSize;
}


#if defined _WIN32 || defined _WIN64

struct WindowsThread
{
	HANDLE threadHandle;
	unsigned int threadID;
};

typedef struct stThread
{
	void* pArgs;
	ThreadFunc pFun;
}stThread;

static unsigned __stdcall ThreadWin(void* args)
{
#if _XBOX_VER >= 200
    // On Xbox 360 you must explicitly assign
    // software threads to hardware threads.
    XSetThreadProcessor( GetCurrentThread(), 2 );
#endif
    
	stThread* pThread = (stThread*)args;
	void *pArgs = pThread->pArgs;
	ThreadFunc pFun = pThread->pFun;

	delete pThread;
	pThread = NULL;

	if (pFun == NULL)
	{
		return 0;
	}
	void* pReturn = pFun(pArgs);
	if (NULL == pReturn)
	{
		return 0;
	}

	return 1;
}

static unsigned __stdcall ThreadWinRun(void* args)
{
#if _XBOX_VER >= 200
    // On Xbox 360 you must explicitly assign
    // software threads to hardware threads.
    XSetThreadProcessor( GetCurrentThread(), 2 );
#endif
    
	Thread* pThread = (Thread*)args;
	pThread->Run();

	//delete pThread;

	return 1;
}

//ªÒµ√µ±«∞µ˜”√’ﬂµƒœﬂ≥ÃID
thread_handle_t GetCurrentThreadHangle()
{
	return (thread_handle_t)GetCurrentThread();
}

Thread::Thread():m_Handle(NULL), m_unStackSize(0), m_bCreateSuspend(false)
{
}

Thread::Thread(bool bSuspend, uint32_t unStackSize):m_Handle(NULL),m_unStackSize(unStackSize),m_bCreateSuspend(bSuspend)
{
}

Thread::~Thread()
{
	Stop();
}

bool Thread::Start(ThreadFunc pFunc,void* pArgs)
{
	if (m_Handle)
	{
		return false;
	}

	WindowsThread *pThreadData = new(std::nothrow) WindowsThread();

	stThread *stInfo = new stThread;
	stInfo->pArgs = pArgs;
	stInfo->pFun = pFunc;

	unsigned int flagCreate = 0;
	if (m_bCreateSuspend)
	{
		flagCreate = CREATE_SUSPENDED;
	}
	pThreadData->threadHandle = (HANDLE)_beginthreadex(NULL, m_unStackSize, ThreadWin, (void*)stInfo, flagCreate, &pThreadData->threadID);

	if (pThreadData->threadHandle == NULL)
	{
		delete pThreadData;
		delete stInfo;
		return false;
	}

	m_Handle = pThreadData;
	return true;
}

bool Thread::Start()
{
	if (m_Handle)
	{
		return false;
	}

	WindowsThread *pThreadData = new(std::nothrow) WindowsThread();

	unsigned int flagCreate = 0;
	if (m_bCreateSuspend)
	{
		flagCreate = CREATE_SUSPENDED;
	}
	pThreadData->threadHandle = (HANDLE)_beginthreadex(NULL, m_unStackSize, ThreadWinRun, this, flagCreate, &pThreadData->threadID);
	if (NULL == pThreadData->threadHandle)
	{
		delete pThreadData;
		return false;
	}

	m_Handle = pThreadData;

	return true;
}

void Thread::Stop()
{
	WindowsThread *pThreadData = (WindowsThread*)pThreadData;
    if (NULL == pThreadData)
    {
        return;
    }
	CloseHandle((HANDLE)pThreadData->threadHandle);
	m_Handle = NULL;
}

void Thread::Resume()
{
	WindowsThread *pThreadData = (WindowsThread*)pThreadData;
    if (NULL == pThreadData)
    {
        return;
    }
	ResumeThread((HANDLE)pThreadData->threadHandle);
}

void Thread::Suspend()
{
	WindowsThread *pThreadData = (WindowsThread*)pThreadData;
    if (NULL == pThreadData)
    {
        return;
    }
	SuspendThread((HANDLE)pThreadData->threadHandle);
}

thread_id_t Thread::GetThreadID() const
{
	WindowsThread *pThreadData = (WindowsThread*)pThreadData;
    if (NULL == pThreadData)
    {
        return 0;
    }
	return pThreadData->threadID;
}

bool Thread::WaitFor() const
{
	WindowsThread *pThreadData = (WindowsThread*)pThreadData;
    if (NULL == pThreadData)
    {
        return false;
    }
	return WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)pThreadData->threadHandle,INFINITE);
}

#elif defined __linux__ || defined __APPLE__

#include <sys/time.h>
#include <errno.h>

//apple系统需要引入的
#if defined __APPLE__
#include <mach/mach.h>
#endif

// 线程对象
struct LinuxThread
{
    pthread_t threadID;            //线程ID
    pthread_attr_t threadAtt;    // 线程属性
};

unsigned long GetTickCount(void)
{
	unsigned long lRet = 0;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	lRet = tv.tv_sec * 1000 + tv.tv_usec/1000;
	return lRet;
}

thread_handle_t GetCurrentThreadHangle()
{
    pthread_t tid = pthread_self();
    
    return tid;
}

Thread::Thread():m_Handle(NULL), m_unStackSize(0), m_bCreateSuspend(false)
{
}

Thread::Thread(bool bSuspend, uint32_t unStackSize):m_Handle(NULL), m_unStackSize(unStackSize), m_bCreateSuspend(bSuspend)
{
}

Thread::~Thread()
{
	Stop();
    
    if (m_Handle)
    {
        LinuxThread *pThreadData = (LinuxThread *)m_Handle;
        delete pThreadData;
        m_Handle = NULL;
        m_unStackSize = 0;
        m_bCreateSuspend = false;
    }
}

static void thread_exit_func(int sig)
{
    pthread_exit(0);
}

bool Thread::Start(ThreadFunc pFunc,void* pArgs)
{
    if (m_Handle)
    {
        return false;
    }
    
    LinuxThread *pThreadData = new(std::nothrow) LinuxThread();
    //struct sched_param schedparam;
    pthread_attr_init(&pThreadData->threadAtt);
    
    //设置栈的大小
    if (m_unStackSize != 0)
    {
        pthread_attr_setstacksize(&pThreadData->threadAtt, m_unStackSize);
    }
    
    int nRet = 0;       //返回标记
#if defined __APPLE__
    
    if (m_bCreateSuspend)
    {
        nRet = pthread_create_suspended_np(&pThreadData->threadID, &pThreadData->threadAtt, pFunc, pArgs);
    }
    else
    {
        nRet = pthread_create(&pThreadData->threadID, &pThreadData->threadAtt, pFunc, pArgs);
    }
    
#elif defined  __ANDROID__
    nRet = pthread_create(&pThreadData->threadID, &pThreadData->threadAtt, pFunc, pArgs);
    
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_func;
    nRet = sigaction(SIGUSR1,&actions,NULL);
#endif
    
    if (nRet)
    {
        delete pThreadData;
        return false;
    }
    
    m_Handle = pThreadData;
    
    return true;
}

static void* pthread_Callback(void* pPara)
{
    Thread* pThis = (Thread *)pPara;
    if (pThis != NULL)
    {
        pThis->Run();
    }
    
    return NULL;
}

bool Thread::Start()
{
    if (m_Handle)
    {
        return false;
    }
    
    void* pArgs = reinterpret_cast<void*>( this );
    int nRet = 1;
    
    LinuxThread *pThreadData = new(std::nothrow) LinuxThread();
    //struct sched_param schedparam;
    pthread_attr_init(&pThreadData->threadAtt);
    
    //设置栈的大小
    if (m_unStackSize != 0)
    {
        pthread_attr_setstacksize(&pThreadData->threadAtt, m_unStackSize);
    }
    
#if defined __APPLE__
    
    if (m_bCreateSuspend)
    {
        nRet = pthread_create_suspended_np(&pThreadData->threadID, &pThreadData->threadAtt, pthread_Callback, pArgs);
    }
    else
    {
        nRet = pthread_create(&pThreadData->threadID, &pThreadData->threadAtt, pthread_Callback, pArgs);
    }
    
//    pthread_attr_t attribute;
//    pthread_attr_setsuspendstate_np(&attribute, 0);
    
#elif defined  __ANDROID__
    //pthread_attr_t attribute;
    //pthread_attr_setsuspendstate_np(&attribute, PTHREAD_CREATE_SUSPENDED_NP);
    
//    pthread_continue_np();
    
    nRet = pthread_create(&pThreadData->threadID, &pThreadData->threadAtt, pthread_Callback, pArgs);
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_func;
    nRet = sigaction(SIGUSR1,&actions,NULL);
#endif
    
    if (nRet)
    {
        delete pThreadData;
        return false;
    }
    
    m_Handle = pThreadData;
    
    return true;
}

void Thread::Stop()
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return;
    }
#ifdef __ANDROID__
    //android has no pthread_cancel
	pthread_kill(pThreadData->threadID, SIGUSR1);
#else
	pthread_cancel(pThreadData->threadID);
#endif
}

void Thread::Resume()
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return;
    }
#ifdef __APPLE__
    mach_port_t mach_thread = pthread_mach_thread_np(pThreadData->threadID);
    thread_resume(mach_thread);
#else
    //pthread_continue_np(m_hThread);
#endif
}

void Thread::Suspend()
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return;
    }
#ifdef __APPLE__
    mach_port_t mach_thread = pthread_mach_thread_np(pThreadData->threadID);
    thread_suspend(mach_thread);
#else
    //pthread_suspend_np(m_hThread);
#endif
}

thread_id_t Thread::GetThreadID() const
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return 0;
    }
#if defined __APPLE__
    return pthread_mach_thread_np(pThreadData->threadID);
#else
    return pThreadData->threadID;
#endif
}

ThreadState Thread::GetThreadState() const
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return Thread_NotFound;
    }
    
    int ret = pthread_kill(pThreadData->threadID, 0);

	if (0 == ret)
		return Thread_Running;
	else if (ESRCH == ret)
		return Thread_Exit;
	else if (EINVAL == ret)
        return Thread_NotFound;

	return Thread_Running;
}

bool Thread::WaitFor() const
{
    LinuxThread *pThreadData = (LinuxThread *)m_Handle;
    if (NULL == pThreadData)
    {
        return false;
    }
    
    pthread_t tid = pThreadData->threadID;
    if (tid)
    {
        void *status;
        int rc = pthread_join(tid, &status);
        if (rc)
            return false;
        return true;
    }
    return false;
}

#endif


NS_BASELIB_END
