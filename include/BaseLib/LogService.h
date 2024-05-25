//
//  LogService.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/20.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#ifndef BASELIB_LOGSERVICE_INCLUDE_HD97762HFD
#define BASELIB_LOGSERVICE_INCLUDE_HD97762HFD

//跨平台轻量级日志服务

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API LogService
{
public:
    static void DebugPrint(const char* msg, ...);
    
    static void InfoPrint(const char* msg, ...);
    
    static void WarnPrint(const char* msg, ...);
    
    static void ErrorPrint(const char* msg, ...);
    
private:
    LogService();
    ~LogService();
    LogService(const LogService&);
    LogService& operator = (const LogService&);
};

#define log_debug(...) { do { baselib::LogService::DebugPrint(__VA_ARGS__); } while(0);}

#define log_info(...) { do { baselib::LogService::InfoPrint(__VA_ARGS__); } while(0);}

#define log_warn(...) { do { baselib::LogService::WarnPrint(__VA_ARGS__); } while(0);}

#define log_error(...) { do { baselib::LogService::ErrorPrint(__VA_ARGS__); } while(0);}

NS_BASELIB_END


#endif /* BASELIB_LOGSERVICE_INCLUDE_HD97762HFD */
