#include "Log.h"

#if defined(_MSC_VER)
#define va_copy(dest, src) (dest = src)
#endif

#if defined WIN32 || defined _WIN64
    #include <Windows.h>

std::atomic<bool> consoleChecked(false);
std::atomic<bool> hasConsole(false);

bool HasConsole() 
{
	if (!consoleChecked.load()) 
    {
		bool result = GetConsoleWindow() != nullptr;
		hasConsole.store(result);
		consoleChecked.store(true);
	}
	return hasConsole.load();
}

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

void Log::LogPrint(LogLevel lev, const char* msg, va_list args)
{
#ifdef __APPLE__
	vprintf(msg, args);
    printf("\n");
#elif defined __ANDROID__
	Android_Print(lev, msg, args);
#elif defined WIN32
	if (HasConsole())
	{
		::vprintf(msg, args);
        ::printf("\n");
	}
	else
	{
        char buf[LogBufSize] = {0};
		::vsnprintf(buf, sizeof(buf), msg, args);
		OutputDebugStringA(buf);
	}
#endif
}

NS_BASELIB_END
