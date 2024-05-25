#ifndef BASELIB_LOG_INCLUDE_FKG569FLDS_H
#define BASELIB_LOG_INCLUDE_FKG569FLDS_H

//跨平台封装轻量日志接口

#include "PreCompile.h"
#include "TaskQueue.h"

NS_BASELIB_BEGIN

class BASELIB_API_HIDE Log
{
public:
	
	/// log levels
	enum LogLevel
    {
		None,
		Error,
		Warn,
		Info,
		Debug,

		NumLevels,
		InvalidLevel
	};

	//
	static int GetNumLoggers();

	//
	static void SetLogLevel(LogLevel l);

	//
	static LogLevel GetLogLevel();

	static void DebugPrint(const char* msg, va_list args);

	static void InfoPrint(const char* msg, va_list args);

	static void WarnPrint(const char* msg, va_list args);

	static void ErrorPrint(const char* msg, va_list args);

	static void AssertMsg(const char* cond, const char* msg, const char* file, int line, const char* func);

private:

	static void vprint(LogLevel lev, const char* msg, va_list args);
    
    static void LogPrint(LogLevel lev, const char* msg, va_list args);
    
    static void* LogThreadFunc(void* pPara);        //日志线程函数
    
    static TaskQueue mLogQueue;                     //日志队列
    
    static Thread mLogThread;                       //日志线程

	static MutexLock mQueueLock;					//队列锁

	static Condition mEmptyCond;
    
    static std::atomic_bool mbLogStart;                          //日志线程是否启动
};


#define o_dbg(...) do { Log::DebugPrint(__VA_ARGS__); } while(0)

#define o_info(...) do { Log::InfoPrint(__VA_ARGS__); } while(0)

#define o_warn(...) do { Log::WarnPrint(__VA_ARGS__); } while(0)

#define o_error(...) do { Log::ErrorPrint(__VA_ARGS__); } while(0)

NS_BASELIB_END

#endif
