#ifndef BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ
#define BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ

//GUID跨平台的封装

#include "PreCompile.h" 

NS_BASELIB_BEGIN


#if !defined(WIN32) && !defined(_WIN64)
struct GUID;
#endif


//创建GUID
BASELIB_API bool CreateGUID(GUID *guid);

//GUID转字符串
BASELIB_API bool GUIDToString(const GUID *guid, std::string& buf);

//guid是否相等
BASELIB_API bool IsGUIDEqual(const GUID &guid1,const GUID& guid2);

NS_BASELIB_END

#endif
