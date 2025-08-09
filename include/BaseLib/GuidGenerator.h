#ifndef BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ
#define BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ

//GUID跨平台的封装

#include "PreCompile.h"

NS_BASELIB_BEGIN

struct GUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};

//创建GUID
BASELIB_API bool CreateGUID(GUID *guid);

//从内存创建
BASELIB_API struct GUID CreateGUIDFromBytes(const uint8_t* pBytes);

//GUID转字符串
BASELIB_API std::string GUIDToString(const GUID &guid);

//guid是否相等
BASELIB_API bool IsGUIDEqual(const GUID &guid1, const GUID& guid2);

NS_BASELIB_END

#endif
