#ifndef CRC32CODE_BASELIB_INCLUDE_H
#define CRC32CODE_BASELIB_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

BASELIB_API uint32_t Calculate_CRC32(const void *pStart, uint32_t uSize);

BASELIB_API uint16_t Calculate_CRC16(const void* pStart, uint32_t uSize);

NS_BASELIB_END

#endif
