//
//  LogService.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/20.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "LogService.h"
#include "Log.h"
#include <sstream>

NS_BASELIB_BEGIN

static std::string InfoPrintEx(Log::LogLevel lev,
	const std::string& file,
	const std::string& func,
	int line,
	const char* msg)
{
	// 获取当前时间戳
	auto now = std::chrono::system_clock::now();
	auto now_time = std::chrono::system_clock::to_time_t(now);

	// 转换时间为本地时间
	std::tm local_tm;
#if OS_WINDOWS
	localtime_s(&local_tm, &now_time);  // Windows安全版本
#else
	localtime_r(&now_time, &local_tm);  // POSIX安全版本
#endif

	// 格式化毫秒
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	// 提取文件名（不含路径）
	std::string base_name = file;
	size_t pos = base_name.find_last_of("/\\");
	if (pos != std::string::npos) 
	{
		base_name = base_name.substr(pos + 1);
	}

	// 格式化日志头
	std::ostringstream header;
	header << "[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
		<< "." << std::setfill('0') << std::setw(3) << ms.count()
		<< "][" << base_name
		<< ":" << func << ":" << line << "]";
#if OS_ANDROID == 0
	switch (lev)
	{
	case Log::LogLevel::Debug:
		header << "[Debug]";
		break;
	case Log::LogLevel::Info:
		header << "[Info]";
		break;
	case Log::LogLevel::Warn:
		header << "[Warn]";
		break;
	case Log::LogLevel::Error:
		header << "[Error]";
		break;
	}
#endif
	header << msg;

	return header.str();
}

void LogService::DebugPrint(const std::string& file, const std::string& func, int line, const char* msg, ...)
{
	std::string fullMsg = InfoPrintEx(Log::LogLevel::Debug, file, func, line, msg);
	const char* szFullMsg = fullMsg.c_str();
    va_list args;
    va_start(args, szFullMsg);
    Log::LogPrint(Log::LogLevel::Debug, szFullMsg, args);
    va_end(args);
}

void LogService::InfoPrint(const std::string& file, const std::string& func, int line, const char* msg, ...)
{
	std::string fullMsg = InfoPrintEx(Log::LogLevel::Info, file, func, line, msg);
	const char* szFullMsg = fullMsg.c_str();
    va_list args;
    va_start(args, szFullMsg);
    Log::LogPrint(Log::LogLevel::Info, szFullMsg, args);
    va_end(args);
}

void LogService::WarnPrint(const std::string& file, const std::string& func, int line, const char* msg, ...)
{
	std::string fullMsg = InfoPrintEx(Log::LogLevel::Warn, file, func, line, msg);
	const char* szFullMsg = fullMsg.c_str();
    va_list args;
    va_start(args, szFullMsg);
    Log::LogPrint(Log::LogLevel::Warn, szFullMsg, args);
    va_end(args);
}

void LogService::ErrorPrint(const std::string& file, const std::string& func, int line, const char* msg, ...)
{
	std::string fullMsg = InfoPrintEx(Log::LogLevel::Error, file, func, line, msg);
	const char* szFullMsg = fullMsg.c_str();
    va_list args;
    va_start(args, szFullMsg);
    Log::LogPrint(Log::LogLevel::Error, szFullMsg, args);
    va_end(args);
}

NS_BASELIB_END
