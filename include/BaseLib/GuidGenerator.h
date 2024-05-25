#ifndef BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ
#define BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ

//GUID生成器，Windows利用现有的，其他平台自己实现

#include "PreCompile.h" 

NS_BASELIB_BEGIN


#if !defined(WIN32) && !defined(_WIN64)
struct GUID;
#endif


//创建GUID
BASELIB_API bool CreateGUID(GUID *guid);

//GUID格式化为字符串
BASELIB_API bool GUIDToString(const GUID *guid, std::string& buf);

BASELIB_API bool IsGUIDEqual(const GUID &guid1,const GUID& guid2);

NS_BASELIB_END

#endif
