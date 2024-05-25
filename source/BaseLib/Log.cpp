#include "Log.h"
#include "MutexLock.h"
#include "TaskRunner.h"
#include "Thread.h"

#if defined(_MSC_VER)
#define va_copy(dest, src) (dest = src)
#endif

#if defined WIN32 || defined _WIN64
    #include <Windows.h>
#endif
#ifdef __ANDROID__
    #include <android/log.h>
#endif

#if defined WIN32 || defined _WIN64
const int LogBufSize = 4 * 1024;
#endif

//一些内部函数
#ifdef __ANDROID__

USING_NS_BASELIB

void Android_Print(Log::LogLevel eLogLevel, const char* format, va_list arglist)
{
    
    if( format == NULL )
        return;
    
    android_LogPriority pri = ANDROID_LOG_DEFAULT;
    switch (eLogLevel)
    {
        case Log::Error:	pri = ANDROID_LOG_ERROR; break;
        case Log::Warn:	pri = ANDROID_LOG_WARN; break;
        case Log::Info:	pri = ANDROID_LOG_INFO; break;
        case Log::Debug: pri = ANDROID_LOG_DEBUG; break;
        default:    pri = ANDROID_LOG_DEFAULT; break;
    }
    __android_log_vprint(pri, "BaseLib", format, arglist);

return;
}

#endif

NS_BASELIB_BEGIN


std::atomic_bool Log::mbLogStart(false);
TaskQueue Log::mLogQueue(0xffffffff);
Thread Log::mLogThread;
MutexLock Log::mQueueLock;
Condition Log::mEmptyCond(&Log::mQueueLock);


baselib::MutexLock lock;


class LogTask : public TaskRunner
{
public:
    Log::LogLevel m_logLevel;           //log级别
    std::string m_msg;                  //日志信息，包括格式化字符串
    va_list m_PraList;                 //日志的可变参数列表
    
    void Run()
    {
        va_list cp_List;
        va_copy(cp_List, m_PraList);
        
#ifdef __APPLE__
        char buf[1024];
        ::vsnprintf(buf, sizeof(buf), m_msg.c_str(), cp_List);
        //printf(buf);
        vprintf(m_msg.c_str(), cp_List);
#elif defined __ANDROID__
        Android_Print(m_logLevel,m_msg.c_str(), cp_List);
#elif defined WIN32
        //
#endif
    }
};

void* Log::LogThreadFunc(void* pPara)
{
    while (1)
    {
        TaskRunnerPtr pTask = mLogQueue.GetHeadTask();
		if (pTask == NULL)
		{
			mEmptyCond.Wait();
		}
        else if (pTask != NULL)
        {
            pTask->Run();
        }
    }
    
    return NULL;
}

void Log::LogPrint(LogLevel lev, const char* msg, va_list args)
{
    //获取日期、时间、文件以及所在行数等信息
    std::string strFile = __FILE__;
    char szLine[20];
    memset(szLine, 0, 20);
    sprintf(szLine, "line : %d",__LINE__);
    std::string strLine = szLine;
    
    std::string strDateTime = __TIMESTAMP__;
    
    std::string strMsg = strDateTime + ", ";
    strMsg += strLine;
    strMsg += ", ";
    strMsg += msg;
    strMsg += "\n";
    
    std::shared_ptr<LogTask> ptrTask = std::make_shared<LogTask>();
    ptrTask->m_msg = strMsg;
    ptrTask->m_logLevel = lev;
    va_copy(ptrTask->m_PraList, args);
    
    mLogQueue.AddTask(ptrTask);
	mEmptyCond.Notify();
    
}

void Log::vprint(LogLevel lev, const char* msg, va_list args)
{
    lock.Lock();
    
    if (1)
    {
#ifdef __ANDROID__
        android_LogPriority pri = ANDROID_LOG_DEFAULT;
        switch (lev)
        {
            case Error:	pri = ANDROID_LOG_ERROR; break;
            case Warn:	pri = ANDROID_LOG_WARN; break;
            case Info:	pri = ANDROID_LOG_INFO; break;
            case Debug: pri = ANDROID_LOG_DEBUG; break;
            default:    pri = ANDROID_LOG_DEFAULT; break;
        }
        __android_log_vprint(pri, "BaseLib", msg, args);
#else
#if defined WIN32 || defined _WIN64
        va_list argsCopy;
        va_copy(argsCopy, args);
#endif
        
        // do the vprintf, this will destroy the original
        // va_list, so we made a copy before if necessary
        std::vprintf(msg, args);
        
#if defined WIN32 || defined _WIN64
        char buf[LogBufSize];
        ::_vsnprintf(buf, sizeof(buf), msg, argsCopy);
#if defined WIN32 || defined _WIN64
        buf[LogBufSize - 1] = 0;
        OutputDebugString(buf);
#elif ORYOL_PNACL
        // replace non-jsonable characters
        char* p = buf;
        do {
            if (*p == '"') *p = '\'';
            else if (*p == '\n') *p = ' ';
        }
        while (*p++);
        char json[LogBufSize + 64];
        std::snprintf(json, sizeof(json), "{\"msg\":\"log\",\"val\":\"%s\"}", buf);
        if (pnaclInstance::HasInstance()) {
            pnaclInstance::Instance()->putMsg(json);
        }
#endif
#endif
#endif
    }
    else
    {
        //for ( l : loggers)
        {
            //l->VPrint(lvl, msg, args);
        }
    }
    lock.UnLock();
}

void Log::DebugPrint(const char* msg, va_list args)
{
    if (!mbLogStart)
    {
        mLogThread.Start(LogThreadFunc, NULL);
        mbLogStart = true;
    }
    Log::LogPrint(Debug, msg, args);
}

void Log::InfoPrint(const char* msg, va_list args)
{
    if (!mbLogStart)
    {
        mLogThread.Start(LogThreadFunc, NULL);
        mbLogStart = true;
    }
    Log::LogPrint(Info, msg, args);
}

void Log::WarnPrint(const char* msg, va_list args)
{
    if (!mbLogStart)
    {
        mLogThread.Start(LogThreadFunc, NULL);
        mbLogStart = true;
    }
    Log::LogPrint(Warn, msg, args);
}

void Log::ErrorPrint(const char* msg, va_list args)
{
    if (!mbLogStart)
    {
        mLogThread.Start(LogThreadFunc, NULL);
        mbLogStart = true;
    }
    Log::LogPrint(Error, msg, args);
}

NS_BASELIB_END
