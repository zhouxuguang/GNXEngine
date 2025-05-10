/**
* @file              PreCompile.h
* @brief             基本的宏定义，头文件包含
* @details           
* @author            zhouxuguang 
* @date              2015年5月15日
* @version           1.0.0.1
* @par               Copyright (c):zhouxuguang
* @par               History:
*/
#ifndef __PRECOMPILE_H_C0C6C4C8_B1C4_49C3_87E4_EEF1ABC862F0__
#define __PRECOMPILE_H_C0C6C4C8_B1C4_49C3_87E4_EEF1ABC862F0__

//基本头文件
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <functional>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <memory>
#include <numeric>
#include <filesystem>

namespace fs = std::filesystem;

#if(__cplusplus >= 201103L)
    #include <unordered_map>
    #include <unordered_set>
#elif (_MSC_VER >= 1500)
    #include <unordered_map>
    #include <unordered_set>

	/*namespace std
	{
		using std::tr1::unordered_map;
		using std::tr1::unordered_set;
		using std::tr1::hash;
	}*/

#else
	#include <tr1/unordered_map>
	#include <tr1/unordered_set>
	namespace std
	{
		using std::tr1::unordered_map;
		using std::tr1::unordered_set;
		using std::tr1::hash;
	}
#endif

#include "SysConstant.h"

typedef std::basic_string<uint16_t> utf16String;
typedef std::basic_string<uint32_t> utf32String;

#if defined _WIN32 || defined WINCE
	#define OS_WINDOWS 1
#elif defined __ANDROID__
    #define OS_ANDROID 1
#elif defined __linux__
	#define OS_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
	#if TARGET_OS_OSX
		#define OS_MACOS 1
	#elif TARGET_OS_IOS
		#define OS_IOS 1
	#endif
#endif

#if OS_MACOS
#include <os/lock.h>
#endif

//跨平台cpu架构的宏定义
#if defined(__ppc__)
    #define TARGET_PPC          1
#elif defined(__ppc64__) 
    #define TARGET_PPC64        1
#elif defined(__i386__)  || defined(_M_IX86)
    #define TARGET_X86          1
#elif defined(__x86_64__) || defined(_M_X64)
    #define TARGET_X86_64       1
#elif defined(__arm__) 
    #define TARGET_ARM          1
#elif defined(__arm64__)
    #define TARGET_ARM64        1
#endif

//对齐内存申明的跨平台定义
#ifdef __GNUC__
    #define DECLARE_ALIGNED(n)   __attribute__((aligned(n)))
#elif defined _MSC_VER
    #define DECLARE_ALIGNED(n) __declspec(align(n))
#endif

typedef uint32_t DECLARE_ALIGNED(4) AlignedUint32;
typedef int32_t DECLARE_ALIGNED(4) AlignedInt32;
typedef uint64_t DECLARE_ALIGNED(8) AlignedUint64;
typedef uint64_t DECLARE_ALIGNED(8) AlignedInt64;

typedef std::vector<uint8_t> ByteVector;
typedef std::shared_ptr<ByteVector> ByteVectorPtr;

//导出宏定义
#if defined _WIN32 || defined __CYGWIN__ || _WIN64
	#ifdef BASELIB_EXPORTS		
		#ifdef __GNUC__
			#define BASELIB_API __attribute__((dllexport))
		#else
			#define BASELIB_API __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define BASELIB_API __attribute__((dllimport))
		#else
			#define BASELIB_API __declspec(dllimport)
		#endif
	#endif
	#define BASELIB_API_HIDE
#else
	#if __GNUC__>=4
		#define BASELIB_API __attribute__((visibility("default")))
		#define BASELIB_API_HIDE __attribute__ ((visibility("hidden")))
	#else
		#define BASELIB_API
		#define BASELIB_API_HIDE
	#endif
#endif

//no export
#undef BASELIB_API
#define BASELIB_API


#if defined _WIN32 ||_WIN64 || defined __CYGWIN__

	#define BASELIB_API_CALL __stdcall
#else
	#if __GNUC__>=4
	#define BASELIB_API_CALL __attribute__((__stdcall__))
	#endif
#endif

//extern "C" 定义
#ifdef __cplusplus
	#define EXTERN_C_START           extern "C" {
	#define EXTERN_C_END             }
#else
	#define EXTERN_C_START
	#define EXTERN_C_END
#endif

//强制内联的定义
#if defined _WIN32 ||_WIN64 || defined __CYGWIN__

	#define FORCE_INLINE __forceinline
#else
	#define FORCE_INLINE __attribute__((always_inline) )
#endif

//#define new new(std::nothrow)

//命名空间定义
#ifdef __cplusplus
    #define NS_BASELIB_BEGIN                     namespace baselib {
    #define NS_BASELIB_END                       }
    #define USING_NS_BASELIB                     using namespace baselib;
#else
    #define NS_BASELIB_BEGIN
    #define NS_BASELIB_END
    #define USING_NS_BASELIB
#endif

// 探测系统位数
#if INTPTR_MAX == INT64_MAX
	#define BASE_IS_64_BIT_CPU 1
#else
	#define BASE_IS_32_BIT_CPU 1
#endif

// Enable futexes on:
//
// - Linux and derivatives (Android, ChromeOS, etc)
// - Windows 8+
//
#if defined(OS_LINUX) || defined(OS_ANDROID)
	// Linux has had futexes for a very long time.  Assume support.
	#define USE_FUTEX 1
#elif OS_WINDOWS
	// Windows has futexes since version 8, which is already end of life (let alone older versions).
	// Assume support.
	#define USE_FUTEX 1
#elif OS_MACOS
     #define USE_FUTEX 1
#endif

#endif // end of file_
