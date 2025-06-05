//
//  LogService.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/20.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#ifndef BASELIB_LOGSERVICE_INCLUDE_HD97762HFD
#define BASELIB_LOGSERVICE_INCLUDE_HD97762HFD

//跨平台轻量级日志d打印服务

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API LogService
{
public:
	static void DebugPrint(const std::string& file, const std::string& func, int line, const char* msg, ...);
    
    static void InfoPrint(const std::string& file, const std::string& func, int line, const char* msg, ...);
    
    static void WarnPrint(const std::string& file, const std::string& func, int line, const char* msg, ...);
    
    static void ErrorPrint(const std::string& file, const std::string& func, int line, const char* msg, ...);
    
private:
    LogService();
    ~LogService();
    LogService(const LogService&);
    LogService& operator = (const LogService&);
};

NS_BASELIB_END

#define LOG_DEBUG(...) do { baselib::LogService::DebugPrint(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(0)

#define LOG_INFO(...) do { baselib::LogService::InfoPrint(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(0)

#define LOG_WARN(...) do { baselib::LogService::WarnPrint(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(0)

#define LOG_ERROR(...) do { baselib::LogService::ErrorPrint(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(0)


#endif /* BASELIB_LOGSERVICE_INCLUDE_HD97762HFD */
