#include "ZigZag.h"

NS_BASELIB_BEGIN

int32_t ZagEncode(uint32_t nZigValue)
{
    int32_t nValue = nZigValue;
	return (-(nValue & 0x01)) ^ ((nValue >> 1) & ~(1 << 31));
}

uint32_t ZigDecode(int32_t nValue)
{
	return (uint32_t)((nValue << 1) ^ (nValue >> 31) );
}

NS_BASELIB_END
