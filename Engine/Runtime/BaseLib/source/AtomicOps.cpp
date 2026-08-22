#include "AtomicOps.h"

#ifdef WIN32
#include <intrin.h>
#endif

NS_BASELIB_BEGIN

bool CBLAtomicSwapPtr(void *volatile *oneValue, void *volatile *otherValue)
{
	if (NULL == oneValue || NULL == otherValue)
	{
		return false;
	}

	// 基于 CAS 循环的原子交换：
	//   1) 记录 oneValue 的当前值 a，读取 otherValue 的当前值 b
	//   2) 尝试把 oneValue 从 a 原子替换为 b
	//   3) 成功则把 otherValue 写为 a，完成交换；失败（期间 oneValue 被并发修改）
	//      则用返回的最新值重试，直至成功
	void *a = *oneValue;
	for (;;)
	{
		void *b = *otherValue;
		void *expected = a;
		if (CBLAtomicCompareAndSwapPtr(expected, b, oneValue))
		{
			*otherValue = a;
			return true;
		}
		a = *oneValue;
	}
}


#if defined(__MACH__) && defined(__APPLE__)    //MACOSX iOS

#include <libkern/OSAtomic.h>

int CBLAtomicAdd(volatile int* ptr, int increment)
{
	return OSAtomicAdd32(increment, (int*)(ptr));
}

int CBLAtomicIncrement(volatile int* ptrValue)
{
    return OSAtomicIncrement32(ptrValue);
}

int CBLAtomicDecrement(volatile int* ptrValue)
{
    return OSAtomicDecrement32(ptrValue);
}

int CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange, int nCompare)
{
    if (OSAtomicCompareAndSwapInt(nCompare, nExchange, ptrValue))
    {
        return nCompare;
    }
    else
        return *ptrValue;
}

bool CBLAtomicCompareAndSwapPtr(void *oldValue, void *newValue, void * volatile *theValue)
{
    return OSAtomicCompareAndSwapPtr(oldValue, newValue, theValue);
}

//windows实现
#elif defined(WIN32) || defined(_WIN64)

int CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange, int nCompare)
{
    // InterlockedCompareExchange(目标, 新值, 旧值) —— 参数顺序与 GCC/OSAtomic 对齐
    return InterlockedCompareExchange((volatile LONG*)ptrValue, nExchange, nCompare);
}

int CBLAtomicIncrement(volatile int* ptrValue)
{
	return InterlockedIncrement((volatile LONG *)ptrValue);
}

int CBLAtomicDecrement(volatile int* ptrValue)
{
	return InterlockedDecrement((volatile LONG *)ptrValue);
}

bool CBLAtomicCompareAndSwapPtr(void *oldValue, void *newValue, void * volatile *theValue)
{
    void *pRet = InterlockedCompareExchangePointer(theValue, newValue, oldValue);
	// pRet 是交换前 *theValue 的值；若等于 oldValue 说明 CAS 成功，返回 true
	return pRet == (void*)oldValue;
}

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

int CBLAtomicAdd(volatile int* ptr, int increment)
{
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
	return InterlockedExchangeAdd((LONG*)(ptr), (LONG)(increment)) + increment;
#else
	return InterlockedExchangeAdd((volatile LONG*)(ptr), (LONG)(increment)) + increment;
#endif
}

#elif defined(__MINGW32__) && defined(__i386__)

int CBLAtomicAdd(volatile int* ptr, int increment)
{
	return InterlockedExchangeAdd((LONG*)(ptr), (LONG)(increment)) + increment;
}

#endif
//Windows实现结束



//GCC实现
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

int CBLAtomicAdd(volatile int* ptr, int increment)
{
	int temp = increment;
	__asm__ __volatile__("lock; xaddl %0,%1"
		: "+r" (temp), "+m" (*ptr)
		: : "memory");
	return temp + increment;
}

int CBLAtomicIncrement(volatile int* ptrValue)
{
    return __sync_add_and_fetch(ptrValue,1);
}

int CBLAtomicDecrement(volatile int* ptrValue)
{
    return __sync_sub_and_fetch(ptrValue,1);
}

int CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange, int nCompare)
{
    return __sync_val_compare_and_swap(ptrValue, nCompare, nExchange);
}

// x86_64 的 CBLAtomicCompareAndSwapPtr 定义（i386 由下方独立 #if 块提供，
// 避免重复定义）
#if defined(__x86_64__)
bool CBLAtomicCompareAndSwapPtr( void *oldValue, void *newValue, void * volatile *theValue )
{
    return __sync_bool_compare_and_swap(theValue, oldValue, newValue);
}
#endif

#else //defined(HAVE_GCC_ATOMIC_BUILTINS)
/* Starting with GCC 4.1.0, built-in functions for atomic memory access are provided. */
/* see http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html */
/* We use a ./configure test to determine whether this builtins are available */
/* as it appears that the GCC 4.1 version used on debian etch is broken when linking */
/* such instructions... */
int CBLAtomicAdd(volatile int* ptr, int increment)
{
	if (increment > 0)
		return __sync_add_and_fetch(ptr, increment);
	else
		return __sync_sub_and_fetch(ptr, -increment);
}

int CBLAtomicIncrement(volatile int* ptrValue)
{
    return __sync_add_and_fetch(ptrValue,1);
}

int CBLAtomicDecrement(volatile int* ptrValue)
{
    return __sync_sub_and_fetch(ptrValue,1);
}

int CBLAtomicCompareSwap(volatile int* ptrValue, int nExchange,int nCompare)
{
    return __sync_val_compare_and_swap(ptrValue,nCompare,nExchange);
}

bool CBLAtomicCompareAndSwapPtr( void *oldValue, void *newValue, void * volatile *theValue )
{
    return __sync_bool_compare_and_swap(theValue, oldValue, newValue);
}

#endif

#if (defined(__i386__) || defined(__i386)) && (!defined(__APPLE__))

bool CBLAtomicCompareAndSwapPtr( void *oldValue, void *newValue, void * volatile *theValue )
{
    return __sync_bool_compare_and_swap(theValue, oldValue, newValue);
}

#endif

#ifdef __IOS__

#endif

#if defined(_M_X64) || defined(__x86_64__)
	#define ARCH_CPU_X86_FAMILY 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
	#define ARCH_CPU_X86_FAMILY 1
#elif defined(__ARMEL__)
	#define ARCH_CPU_ARM_FAMILY 1
#elif defined(__aarch64__)
	#define ARCH_CPU_ARM64_FAMILY 1
#elif defined(__ppc__) || defined(__powerpc__) || defined(__powerpc64__)
	#define ARCH_CPU_PPC_FAMILY 1
#elif defined(__mips__)
	#define ARCH_CPU_MIPS_FAMILY 1
#endif

// Define MemoryBarrier() if available
// Windows on x86
#if defined(WIN32) && defined(_MSC_VER) && defined(ARCH_CPU_X86_FAMILY)
// windows.h already provides a MemoryBarrier(void) macro
// http://msdn.microsoft.com/en-us/library/ms684208(v=vs.85).aspx

// Mac OS
#elif defined(__APPLE__)
inline void MemoryBarrier() 
{
	OSMemoryBarrier();
}

// Gcc on x86
#elif defined(ARCH_CPU_X86_FAMILY) && defined(__GNUC__)
inline void MemoryBarrier() 
{
	// See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
	// this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
	__asm__ __volatile__("" : : : "memory");
}

// Sun Studio
#elif defined(ARCH_CPU_X86_FAMILY) && defined(__SUNPRO_CC)
inline void MemoryBarrier() 
{
	// See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
	// this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
	asm volatile("" : : : "memory");
}

// ARM Linux
#elif defined(ARCH_CPU_ARM_FAMILY) && defined(__linux__)
typedef void (*LinuxKernelMemoryBarrierFunc)(void);
// The Linux ARM kernel provides a highly optimized device-specific memory
// barrier function at a fixed memory address that is mapped in every
// user-level process.
//
// This beats using CPU-specific instructions which are, on single-core
// devices, un-necessary and very costly (e.g. ARMv7-A "dmb" takes more
// than 180ns on a Cortex-A8 like the one on a Nexus One). Benchmarking
// shows that the extra function call cost is completely negligible on
// multi-core devices.
//
inline void MemoryBarrier() 
{
	(*(LinuxKernelMemoryBarrierFunc)0xffff0fa0)();
}

// ARM64
#elif defined(ARCH_CPU_ARM64_FAMILY)
inline void MemoryBarrier() 
{
	asm volatile("dmb sy" : : : "memory");
}

// PPC
#elif defined(ARCH_CPU_PPC_FAMILY) && defined(__GNUC__)
inline void MemoryBarrier() 
{
	// TODO for some powerpc expert: is there a cheaper suitable variant?
	// Perhaps by having separate barriers for acquire and release ops.
	asm volatile("sync" : : : "memory");
}

// MIPS
#elif defined(ARCH_CPU_MIPS_FAMILY) && defined(__GNUC__)
inline void MemoryBarrier() 
{
	__asm__ __volatile__("sync" : : : "memory");
}

#endif

NS_BASELIB_END
