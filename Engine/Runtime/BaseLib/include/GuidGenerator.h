#ifndef BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ
#define BASELIB_GUIDGENERATOR_INCLUDE_H_45FG6HJ

//GUID跨平台的封装

#include "PreCompile.h"

NS_BASELIB_BEGIN

struct NXGUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};

//创建GUID
BASELIB_API bool CreateGUID(NXGUID *guid);

//从内存创建
BASELIB_API struct NXGUID CreateGUIDFromBytes(const uint8_t* pBytes);

//GUID转字符串
BASELIB_API std::string GUIDToString(const NXGUID &guid);

//guid是否相等
BASELIB_API bool IsGUIDEqual(const NXGUID &guid1, const NXGUID& guid2);

NS_BASELIB_END

#endif
