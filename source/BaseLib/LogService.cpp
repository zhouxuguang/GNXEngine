//
//  LogService.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/20.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "LogService.h"
#include "Log.h"

NS_BASELIB_BEGIN

void LogService::DebugPrint(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    Log::DebugPrint(msg,args);
    va_end(args);
}

void LogService::InfoPrint(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    Log::InfoPrint(msg,args);
    va_end(args);
}

void LogService::WarnPrint(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    Log::WarnPrint(msg,args);
    va_end(args);
}

void LogService::ErrorPrint(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    Log::ErrorPrint(msg,args);
    va_end(args);
}

NS_BASELIB_END
