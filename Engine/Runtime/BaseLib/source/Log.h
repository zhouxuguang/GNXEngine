#ifndef BASELIB_LOG_INCLUDE_FKG569FLDS_H
#define BASELIB_LOG_INCLUDE_FKG569FLDS_H

//跨平台封装轻量日志接口

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API_HIDE Log
{
public:
	/// log levels
	enum LogLevel
    {
		Error,
		Warn,
		Info,
		Debug,

		NumLevels,
		InvalidLevel
	};

	static void LogPrint(LogLevel lev, const char* msg, va_list args);
};

NS_BASELIB_END

#endif
