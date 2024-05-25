
#ifndef BASELIB_BYTESWAP_INCLUDE_H_KG58GYUFT675JK
#define BASELIB_BYTESWAP_INCLUDE_H_KG58GYUFT675JK

//字节交换、大小尾端以及机器的字节序和大小尾端之间的转换的基础类

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API BytesSwap
{
public:
	static uint16_t SwapBytes( uint16_t num);

	static int16_t SwapBytes( int16_t num);

	static uint32_t SwapBytes(uint32_t num);

	static int32_t SwapBytes( int32_t num);

	static float SwapBytes( float num);

	static double SwapBytes(double num);
    
    //32位整数转换
    static uint32_t SwapInt32LittleToHost(uint32_t arg);
    
    static uint32_t SwapInt32BigToHost(uint32_t arg);
    
    static uint32_t SwapInt32HostToLittle(uint32_t arg);
    
    static uint32_t SwapInt32HostToBig(uint32_t arg);
    
    //16位整数转换
    static  uint16_t SwapInt16LittleToHost(uint16_t arg);
    
    static  uint16_t SwapInt16BigToHost(uint16_t arg);
    
    static  uint16_t SwapInt16HostToLittle(uint16_t arg);
    
    static  uint16_t SwapInt16HostToBig(uint16_t arg);
    
private:
    BytesSwap();
    ~BytesSwap();
    BytesSwap(const BytesSwap&);
    BytesSwap& operator = (const BytesSwap&);
};

NS_BASELIB_END

#endif
