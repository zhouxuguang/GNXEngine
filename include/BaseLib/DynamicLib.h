/*******************************************************************************
@ 版权所有(C) 周旭光 2014
@ 文件名称	: DynamicLib.h
@ 当前版本	: 1.0.0.1
@ 作    者	: guang (zhouxuguang236@126.com)
@ 设计日期	: 2014年12月5日
@ 内容摘要	: 
@ 修改记录	: 
@ 日    期		版    本		修改人		修改摘要

********************************************************************************/
#ifndef __DYNAMICLIB_H_INCLUDE
#define __DYNAMICLIB_H_INCLUDE

#include "PreCompile.h"

NS_BASELIB_BEGIN

EXTERN_C_START

BASELIB_API void *LoadLib(const char* strPath);

BASELIB_API void *GetSym(void *hLib,const char* strName); 

BASELIB_API void FreeLib(void *hLib);

EXTERN_C_END

NS_BASELIB_END


#endif
