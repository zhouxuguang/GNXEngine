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

namespace
{
	// 源文件按 /utf-8 编译，输出的是 UTF-8 字节；而 Windows 控制台默认按系统 ANSI
	// 代码页（简体中文系统为 936/GBK）解码，导致中文乱码。这里在模块加载阶段
	// （main 之前）把控制台代码页切换到 UTF-8，使进程内所有输出（含 printf）都能
	// 正确显示中文，无需调用方做任何事。
	struct ConsoleUtf8Initializer
	{
		ConsoleUtf8Initializer()
		{
			::SetConsoleOutputCP(CP_UTF8);
			::SetConsoleCP(CP_UTF8);
		}
	};

	const ConsoleUtf8Initializer g_consoleUtf8Initializer;
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
