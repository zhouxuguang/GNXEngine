
#ifndef BASELIB_SYSTEMINFO_INCLUDE_H
#define BASELIB_SYSTEMINFO_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

//获得当前设备的DPI
BASELIB_API float GetDPI();

BASELIB_API double GetCPUUserTime();

//获得当前程序EXE所在的目录（最后不带路径分隔符）
BASELIB_API std::string GetExePath();

//获得程序的名称
BASELIB_API std::string GetExeName();

//获得APP所使用的物理内存（单位M）
BASELIB_API double GetAppMemory();

//获得APP的CPU使用率（单位  百分数  如99%   则返回99）
BASELIB_API double GetAppCpuUsage();

NS_BASELIB_END

#endif
