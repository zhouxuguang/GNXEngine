/**
* @file              PreCompile.h
* @brief             预编译处理
* @details           
* @author            zxg 
* @date              2015年5月15日
* @version           1.0.0.1
* @par               Copyright (c):周旭光 2015
* @par               History:
*/
#ifndef __PRECOMPILE_H_C0C6C4C8_B1C4_49C3_87E4_EEF1ABC862F0__
#define __PRECOMPILE_H_C0C6C4C8_B1C4_49C3_87E4_EEF1ABC862F0__

//预先包含一些头文件
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <functional>
#include <algorithm>
#include <numeric>

#if(__cplusplus >= 201103L)
    #include <unordered_map>
    #include <unordered_set>
#elif (_MSC_VER >= 1500)
    #include <unordered_map>
    #include <unordered_set>

	namespace std
	{
		using std::tr1::unordered_map;
		using std::tr1::unordered_set;
		using std::tr1::hash;
	}

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

//引入引擎内部的头文件
#include "SysConstant.h"

//定义utf16和utf32
typedef std::basic_string<uint16_t> utf16String;
typedef std::basic_string<uint32_t> utf32String;

#if defined _WIN32 || defined _WIN64 || defined WINCE
	#define OS_WINDOWS
#elif defined __ANDROID__
    #define OS_ANDROID
#elif defined __linux__
	#define OS_LINUX
#elif (defined(__MACH__) && defined(__APPLE__) )
    #define OS_MACOSX
#elif defined __IOS__
    #define OS_IOS
#endif

//声明对齐的内存
#ifdef __GNUC__
    #define DECLARE_ALIGNED(n)   __attribute__((aligned(n)))
#elif defined _MSC_VER
    #define DECLARE_ALIGNED(n) __declspec(align(n))
#endif

typedef uint32_t DECLARE_ALIGNED(4) AlignedUint32;
typedef int32_t DECLARE_ALIGNED(4) AlignedInt32;
typedef uint64_t DECLARE_ALIGNED(8) AlignedUint64;
typedef uint64_t DECLARE_ALIGNED(8) AlignedInt64;



//导出宏声明
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

//C链接方式宏定义
#ifdef __cplusplus
	#define EXTERN_C_START           extern "C" {
	#define EXTERN_C_END             }
#else
	#define EXTERN_C_START
	#define EXTERN_C_END
#endif

//强制内联的宏定义，并不一定能成功
#if defined _WIN32 ||_WIN64 || defined __CYGWIN__

	#define FORCE_INLINE __forceinline
#else
	#define FORCE_INLINE __attribute__((always_inline) )
#endif

//声明不跑出异常的new 运算符
//#define new new(std::nothrow)

//命名空间声明
#ifdef __cplusplus
    #define NS_BASELIB_BEGIN                     namespace baselib {
    #define NS_BASELIB_END                       }
    #define USING_NS_BASELIB                     using namespace baselib;
#else
    #define NS_BASELIB_BEGIN
    #define NS_BASELIB_END
    #define USING_NS_BASELIB
#endif



#endif // end of file_
