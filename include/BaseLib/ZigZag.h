#ifndef ZIGZAG_INCLUDE_H_BASELIB
#define ZIGZAG_INCLUDE_H_BASELIB

#include "PreCompile.h"

NS_BASELIB_BEGIN

BASELIB_API int32_t ZagEncode(uint32_t nZigValue);

BASELIB_API uint32_t ZigDecode(int32_t nValue);

NS_BASELIB_END

#endif
